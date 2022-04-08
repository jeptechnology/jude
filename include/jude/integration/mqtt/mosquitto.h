/*
 * The MIT License (MIT)
 * Copyright © 2022 James Parker
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal 
 * in the Software without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
   This serves as an example of how to interact with an MQTT broker using libmosquitto
   Please treat this code as a development template. 
   You can intergrate your own MQTT integration layer if you so wish. 
*/

#pragma once

#include <mosquitto.h>
#include <thread>
#include <vector>
#include <map>
#include <jude/database/Database.h>
#include <jude/core/cpp/Stream.h>
#include <jude/core/cpp/FieldMask.h>

namespace jude 
{
   class MQTT
   {
      static constexpr const char *Added   = "added";
      static constexpr const char *Updated = "updated";
      static constexpr const char *Deleted = "deleted";

      std::string       m_clientId;
      jude::NotifyQueue m_notifyQueue;
      bool              m_isStopping;
      bool              m_isConnected;
      struct mosquitto *m_mqtt;
      std::vector<SubscriptionHandle>    m_unsubscribers;
      std::unique_ptr<std::thread> m_mqttProcessingThread;
      std::unique_ptr<std::thread> m_mqttPublishingThread;
      std::map<std::string, DatabaseEntry*> m_mqttSubscribers;

      void HandleMqttConnection(int result)
      {
         m_isConnected = (result == 0);
         std::cout << "MQTT is now " << (m_isConnected ? "connected" : "disconnected") << std::endl;
         
         if (m_isConnected)
         {
            for (auto& sub : m_mqttSubscribers)
            {
               auto pattern = sub.first + "#";
               mosquitto_subscribe(m_mqtt, nullptr, pattern.c_str(), 0);         
            }
         }     
      }

      void HandleDatabaseChange(const std::string& rootTopic, const std::string& path, const jude::Notification<jude::Object>& info)
      {
         // publish a "delete" from top level
         if (info.IsDeleted())
         {
            auto topic = rootTopic + "deleted" + path;
            mosquitto_publish(m_mqtt, NULL, topic.c_str(), 4, "null", 0, false);   
            return;
         }

         // publish a "added" from top level
         if (info.IsNew())
         {
            auto topic = rootTopic + "added" + path;
            auto json = info->ToJSON(); 
            mosquitto_publish(m_mqtt, NULL, topic.c_str(), json.length(), json.c_str(), 0, false);   
            return;
         }

         // publish "updated" for all changed fields
         if (!info.IsDeleted())
         {
            auto fieldList = info->Type().field_list;
            auto prefix = rootTopic + "updated" + path + "/";

            for (const auto& index : info->GetChanges().AsVector())
            {
               auto topic = prefix + info->FieldName(index);
               auto value = info->ToString(index);
               mosquitto_publish(m_mqtt, NULL, topic.c_str(), value.length(), value.c_str(), 0, fieldList[index].persist);   
            }   
         }
      }

      void HandleIncomingMqttMessage(const struct mosquitto_message *message)
      {
         std::string topic = message->topic;

         for (auto& subscriber : m_mqttSubscribers)
         {
            if (topic.compare(0, subscriber.first.size(), subscriber.first) == 0)
            {
               jude::RestfulResult result;
               // Check for added / updated / deleted
               auto suffix = topic.substr(subscriber.first.size());
               if (topic.compare(subscriber.first.size(), strlen(Updated), Updated) == 0)
               {
                  auto json = std::string(reinterpret_cast<char *>(message->payload), message->payloadlen);
                  auto path = topic.substr(subscriber.first.size() + strlen(Updated));
                  result = subscriber.second->RestPatchString(path.c_str(), json.c_str(), jude_user_Admin);
               }
               else if (topic.compare(subscriber.first.size(), strlen(Added), Added) == 0)
               {
                  // TODO: Do we need to post first?
                  auto json = std::string(reinterpret_cast<char *>(message->payload), message->payloadlen);
                  auto path = topic.substr(subscriber.first.size() + strlen(Added));
                  result = subscriber.second->RestPostString(path.c_str(), json.c_str(), jude_user_Admin);
               }
               else if (topic.compare(subscriber.first.size(), strlen(Deleted), Deleted) == 0)
               {
                  auto path = topic.substr(subscriber.first.size() + strlen(Deleted));
                  result = subscriber.second->RestDelete(path.c_str(), jude_user_Admin);
               }
               else
               {
                  std::cerr << "Unexpected topic: " << topic << std::endl;
                  break;
               }

               if (!result)
               {
                  std::cerr << "Error in MQTT update: " << result.GetDetails() << std::endl;
               }

               break;
            }
         }

      }

      void ProcessingLoop()
      {
         int rc;

         while(0 == (rc = mosquitto_loop(m_mqtt, 1000, 1)))
         {
            if (m_isStopping)
            {
               return;
            }   
         }
      }

      void ReconnectingLoop(const std::string& mqtthost, int mqttport)
      {
         while (mosquitto_connect(m_mqtt, mqtthost.c_str(), mqttport, 60))
         {
            for (int i = 10; i > 0; i--)
            {
               if (m_isStopping)
               {
                  return;
               }   
               std::this_thread::sleep_for(std::chrono::seconds(10));
            }
         }
      }

      void MainPublishingLoop(const std::string& mqtthost, int mqttport)
      {
         if (!m_isStopping)
         {
            ReconnectingLoop(mqtthost, mqttport);                
         }
         if (!m_isStopping)
         {
            ProcessingLoop();
         }
      }

      void Stop()
      {
         m_isStopping = true;
         if (m_mqttProcessingThread)
         {
            m_mqttProcessingThread->join();            
         }
         if (m_mqttPublishingThread)
         {
            m_mqttPublishingThread->join();
         }
      }

   public:
      MQTT(std::string mqttClientId, const std::string& mqtthost, int mqttport)
         : m_clientId(mqttClientId)
         , m_notifyQueue("MQTT", 1024)
         , m_isStopping(false)
         , m_isConnected(false)
      {
         mosquitto_lib_init();
         
         m_mqtt = mosquitto_new(m_clientId.c_str(), true, this);
         if (!m_mqtt)
         {
            return;
         }

         mosquitto_connect_callback_set(m_mqtt, [](struct mosquitto *mosq, void *obj, int result)
         {
            reinterpret_cast<MQTT*>(obj)->HandleMqttConnection(result);
         });

         mosquitto_message_callback_set(m_mqtt, [](struct mosquitto *mosq, void *obj, const struct mosquitto_message *message){
            reinterpret_cast<MQTT*>(obj)->HandleIncomingMqttMessage(message);
         });

         m_mqttProcessingThread = std::make_unique<std::thread>([this, mqtthost, mqttport] { 
            while (!m_isStopping)
            {
               MainPublishingLoop(mqtthost, mqttport);
            }
         });

         m_mqttPublishingThread = std::make_unique<std::thread>([this] { 
            while (!m_isStopping)
            {
               m_notifyQueue.Process(100);
            }
         });
      }

      ~MQTT()
      {
         for (auto &unsubscriber : m_unsubscribers)
         {
            unsubscriber.Unsubscribe();
         }
         Stop();
      }

      void StartPublishing(DatabaseEntry& db, std::string topicPrefix, jude_user_t user = jude_user_Public)
      {
         m_unsubscribers.push_back(db.SubscribeToAllPaths(
            "", 
            [this, topicPrefix] (const std::string& topic, const jude::Notification<jude::Object>& info) { 
               HandleDatabaseChange(topicPrefix, topic, info);
            }, 
            FieldMask::ForUser(user), 
            m_notifyQueue));
      }

      void StartSubscribing(std::string topicPrefix, DatabaseEntry& db)
      {
         m_mqttSubscribers[topicPrefix] = &db;
         if (m_isConnected)
         {
            auto pattern = topicPrefix + "#";
            mosquitto_subscribe(m_mqtt, nullptr, pattern.c_str(), 0);         
         }
         else
         {
            // This subscription will be done when connection is established  
         }
      }
   };
}
