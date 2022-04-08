#include <gtest/gtest.h>
#include <inttypes.h>

#include "../core/test_base.h"
#include "jude/restapi/jude_browser.h"
#include "autogen/alltypes_test/AllOptionalTypes.h"
#include "autogen/alltypes_test/AllRepeatedTypes.h"
#include "jude/database/Collection.h"

#include <algorithm>

using namespace std;
using namespace jude;

class CollectionIteratorTests : public JudeTestBase
{
   static constexpr size_t CollectionCapacity = 50;

public:
   Collection<SubMessage> m_collection;
   const Collection<SubMessage>& m_constCollection;

   CollectionIteratorTests()
      : m_collection("MyCollection", CollectionCapacity, jude_user_Root)
      , m_constCollection(m_collection)
   {
   }
};

TEST_F(CollectionIteratorTests, collection_counting)
{
   ASSERT_EQ(0, std::count_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return true;}));

   m_collection.Post()->Set_substuff1("A");
   m_collection.Post()->Set_substuff1("B");
   m_collection.Post()->Set_substuff1("C");

   ASSERT_EQ(3, std::count_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return true;}));
   ASSERT_EQ(1, std::count_if(std::begin(m_collection), std::end(m_collection), [](auto const& s){ return s.Get_substuff1() == "A"; }));
}

TEST_F(CollectionIteratorTests, collection_finding)
{
   m_collection.Post(123)->Set_substuff1("A");
   m_collection.Post(456)->Set_substuff1("B").Set_substuff3(true);
   m_collection.Post(789)->Set_substuff1("C");

   ASSERT_TRUE(std::find_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff1() == "A";}));
   ASSERT_TRUE(std::find_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff1() == "B";}));
   ASSERT_TRUE(std::find_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff1() == "C";}));
   ASSERT_FALSE(std::find_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff1() == "D";}));

   auto iter = std::find_if(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff3();});
   ASSERT_TRUE(iter);
   ASSERT_EQ(456, iter->Id());
   ASSERT_EQ("B", iter->Get_substuff1());

   iter = std::find_if_not(std::begin(m_collection), std::end(m_collection), [](auto s){ return s.Get_substuff3();});
   ASSERT_TRUE(iter);
   ASSERT_EQ(123, iter->Id());
   ASSERT_EQ("A", iter->Get_substuff1());

   // Search *after* iter, we should find "C"
   iter = std::find_if_not(++iter, std::end(m_collection), [](auto s){ return s.Get_substuff3();});
   ASSERT_TRUE(iter);
   ASSERT_EQ(789, iter->Id());
   ASSERT_EQ("C", iter->Get_substuff1());
}

TEST_F(CollectionIteratorTests, collection_all_any_none_of)
{
   m_collection.Post(123)->Set_substuff1("A");
   m_collection.Post(456)->Set_substuff1("B").Set_substuff2(123);
   m_collection.Post(789)->Set_substuff1("C");

   EXPECT_TRUE(std::all_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff1(); }));
   EXPECT_TRUE(std::any_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff1(); }));
   EXPECT_FALSE(std::none_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff1(); }));
   
   EXPECT_FALSE(std::all_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff2(); }));
   EXPECT_TRUE(std::any_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff2(); }));
   EXPECT_FALSE(std::none_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff2(); }));

   EXPECT_FALSE(std::all_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff3(); }));
   EXPECT_FALSE(std::any_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff3(); }));
   EXPECT_TRUE(std::none_of(begin(m_collection), end(m_collection), [] (auto& o) { return o.Has_substuff3(); }));
}

TEST_F(CollectionIteratorTests, collection_for_each)
{
   m_collection.Post(123)->Set_substuff1("A");
   m_collection.Post(456)->Set_substuff1("B").Set_substuff2(123);
   m_collection.Post(789)->Set_substuff1("C");

   string temp;
   for_each(begin(m_collection), end(m_collection), [&] (auto& o) { temp += o.Get_substuff1(); });
   EXPECT_EQ(temp, "ABC");
}
