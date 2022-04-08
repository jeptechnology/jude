#pragma once

#include <cstdint>
#include <array>
#include <type_traits>
#include <iostream>

namespace
{
   namespace VT100
   {   
      enum class DisplayAttribute : char
      {
         Reset,
         Bright,
         Dim,
         Underscore,
         Blink,
         Reverse,
         Hidden,
         
         MAX,
         Default = Reset
      };
      
      enum class ColourIndex : char
      {
         Black,
         Red,
         Green,
         Yellow,
         Blue,
         Magenta,
         Cyan,
         White,
         
         MAX,
         DefaultBackground = Black,
         DefaultForeground = White
      };

      class SetDisplay
      {
         std::ostream& m_output;
         bool m_noAttributesSent = true;
         bool m_enabled = true;

         void Write(const char *stringToWrite)
         {
            if (m_enabled)
            {
               m_output << stringToWrite;
            }
         }
         
         void Write(const char *characters, size_t count)
         {
            if (m_enabled)
            {
               m_output.write(characters, count);
            }
         }

         void EndPreviousAttribute()
         {
            if (m_noAttributesSent)
            {
               m_noAttributesSent = false;
               return;
            }
            Write(";");
         }

   public:

         SetDisplay(std::ostream& output)
         : m_output(output)
         {
            Write("\x1B[");
         }
         
         ~SetDisplay()
         {
            Write("m");
         }

         SetDisplay &SetColour(ColourIndex colour = ColourIndex::White)
         {
            EndPreviousAttribute();
            Write("3");
            char secondCharOfColour = '0' + static_cast<unsigned char>(colour);
            Write(&secondCharOfColour, 1);
            return *this;
         }
         
         SetDisplay &SetBackgroundColour(ColourIndex colour = ColourIndex::Black)
         {
            EndPreviousAttribute();
            Write("4");
            char secondCharOfColour = '0' + static_cast<unsigned char>(colour);
            Write(&secondCharOfColour, 1);
            return *this;
         }
         
         SetDisplay &SetAttribute(DisplayAttribute attribute)
         {
            EndPreviousAttribute();
            char attributeAsChar = '0' + static_cast<unsigned char>(attribute);
            Write(&attributeAsChar, 1);
            return *this;
         }

         SetDisplay &SetDefault()
         {
            SetColour(VT100::ColourIndex::DefaultForeground);
            SetBackgroundColour(VT100::ColourIndex::DefaultBackground);
            SetAttribute(VT100::DisplayAttribute::Default);
            return *this;
         }
      };
   }

   //nasty workaround to be removed once we have a testing compiler which has constexpr
   #ifdef _MSC_VER
   #define underlying_cast(value) (static_cast<std::underlying_type<decltype(value)>::type>(value)) 
   #else
   template<typename T>
   constexpr typename std::underlying_type<T>::type underlying_cast(T value)
   {
      return static_cast<typename std::underlying_type<T>::type>(value);
   }
   #endif
}

class JsonPrettyPrinter
{
public:
   enum class JSONParseState : std::uint8_t
   {
      Await,
      InNameString,
      NameStringEscape,
      NameEnd,
      AwaitValue,
      InValueString,
      ValueStringEscape,
      ValueStringEnd,
      ConsumeNumeric,
      ConsumeFloat,
      ConsumeBool,

      MAX
   };

   using S = JSONParseState; //private alias to make the transition table simpler

   enum class JSONParseSymbol :std::uint8_t
   {
      Numeric,                   // ::= 0|1|2|[...]|9
      Alphabetic,                // ::= a|b|c|[...]|z|A|B|C|[...]|Z          
      DoubleQuote,               // ::= "
      Escape,                    // ::= \    /**/
      ObjectStart,               // ::= {
      ObjectEnd,                 // ::= }
      ListStart,                 // ::= [
      ListEnd,                   // ::= ]
      Separator,                 // ::= ,
      DelaratorEnd,              // ::= :
      DecimalPoint,              // ::= .
      WhiteSpace,                // ::= [' ']|['\t']
      Signedness,                // ::= +|-
      OtherPrintableSymbol,      // ::= !|@|#|$|[...]

      MAX
   };

private:
   using TransitionTable = std::array<std::array<JSONParseState, underlying_cast(JSONParseSymbol::MAX)>, underlying_cast(JSONParseState::MAX)>;
   using TTRow = std::array<JSONParseState, underlying_cast(JSONParseSymbol::MAX)>;

   static const TransitionTable g_transitions;

   static JSONParseSymbol Classify(char);
   JSONParseState m_state = JSONParseState::Await;
   bool m_enabled = true;

   std::ostream &m_output;

   void ColourForStateAndWrite(char);
   void ConsumeChar(char character);
   JSONParseState GetNextState(JSONParseSymbol);   
   void SideEffectsBeforeChar(JSONParseSymbol, JSONParseState);
   void SideEffectsAfterChar(JSONParseSymbol, JSONParseState);
   static const char * const g_tabString;
   
   unsigned int m_pushDepth = 0;

   //Formatting side effects
   void NewLine();
   void StartBlock();
   void InsertSpace();
   void EndBlockBefore();
   void Reset();
   void WriteChar(char);
   void TabOut();

public:
   JsonPrettyPrinter(std::ostream &outputStream);
   ~JsonPrettyPrinter();

   void Print(const std::string& json);
};

