#include "JsonPrettyPrinter.h"

using JSONParseState = JsonPrettyPrinter::JSONParseState;
using VT100::SetDisplay;

JsonPrettyPrinter::JsonPrettyPrinter(std::ostream &outputStream)
   : m_output(outputStream)
{
   Reset();
}

JsonPrettyPrinter::~JsonPrettyPrinter()
{
   Reset();
}

const JsonPrettyPrinter::TransitionTable JsonPrettyPrinter::g_transitions {
      // Numeric            | Alphabetic        | DoubleQuote      | Escape              | ObjectStart     | ObjectEnd       | ListStart       | ListEnd          | Separator       | DelaratorEnd    | DecimalPoint    | Whitespace           | Signedness          | OtherPrintableSymbol
   TTRow{ S::ConsumeNumeric,   S::ConsumeBool,     S::InNameString,   S::MAX,               S::Await,         S::Await,         S::Await,         S::Await,          S::Await,         S::MAX,           S::MAX,           S::Await,              S::ConsumeNumeric,    S::MAX               },  //Await
   TTRow{ S::InNameString,     S::InNameString,    S::NameEnd,        S::NameStringEscape,  S::InNameString,  S::InNameString,  S::InNameString,  S::InNameString,   S::InNameString,  S::InNameString,  S::InNameString,  S::InNameString,       S::InNameString,      S::InNameString      },  //InNameString
   TTRow{ S::InNameString,     S::InNameString,    S::InNameString,   S::InNameString,      S::InNameString,  S::InNameString,  S::InNameString,  S::InNameString,   S::InNameString,  S::InNameString,  S::InNameString,  S::InNameString,       S::InNameString,      S::InNameString      },  //NameStringEscape
   TTRow{ S::MAX,              S::MAX,             S::MAX,            S::MAX,               S::MAX,           S::MAX,           S::MAX,           S::MAX,            S::MAX,           S::AwaitValue,    S::MAX,           S::NameEnd,            S::MAX,               S::MAX               },  //NameEnd
   TTRow{ S::ConsumeNumeric,   S::ConsumeBool,     S::InValueString,  S::MAX,               S::Await,         S::MAX,           S::Await,         S::MAX,            S::MAX,           S::MAX,           S::MAX,           S::AwaitValue,         S::ConsumeNumeric,    S::MAX               },  //AwaitValue
   TTRow{ S::InValueString,    S::InValueString,   S::ValueStringEnd, S::ValueStringEscape, S::InValueString, S::InValueString, S::InValueString, S::InValueString,  S::InValueString, S::InValueString, S::InValueString, S::InValueString,      S::InValueString,     S::InValueString     },  //InValueString
   TTRow{ S::InValueString,    S::InValueString,   S::InValueString,  S::InValueString,     S::InValueString, S::InValueString, S::InValueString, S::InValueString,  S::InValueString, S::InValueString, S::InValueString, S::InValueString,      S::InValueString,     S::InValueString     },  //ValueStringEscape
   TTRow{ S::MAX,              S::MAX,             S::MAX,            S::MAX,               S::MAX,           S::Await,         S::MAX,           S::MAX,            S::Await,         S::MAX,           S::MAX,           S::Await,              S::MAX,               S::MAX               },  //ValueStringEnd
   TTRow{ S::ConsumeNumeric,   S::MAX,             S::MAX,            S::MAX,               S::MAX,           S::Await,         S::MAX,           S::Await,          S::Await,         S::MAX,           S::ConsumeFloat,  S::Await,              S::MAX,               S::MAX               },  //ConsumeNumeric
   TTRow{ S::ConsumeFloat,     S::MAX,             S::MAX,            S::MAX,               S::MAX,           S::Await,         S::MAX,           S::Await,          S::Await,         S::MAX,           S::MAX,           S::Await,              S::MAX,               S::MAX               },  //ConsumeFloat
   TTRow{ S::MAX,              S::ConsumeBool,     S::MAX,            S::MAX,               S::MAX,           S::Await,         S::MAX,           S::Await,          S::Await,         S::MAX,           S::MAX,           S::Await,              S::MAX,               S::MAX               }   //ConsumeBool
};


const char * const JsonPrettyPrinter::g_tabString = "   ";

void JsonPrettyPrinter::NewLine()
{
   WriteChar('\r');
   WriteChar('\n');
   TabOut();
}

void JsonPrettyPrinter::StartBlock()
{
   ++m_pushDepth;
   NewLine();
}

void JsonPrettyPrinter::InsertSpace()
{
   WriteChar(' ');
}

void JsonPrettyPrinter::EndBlockBefore()
{
   --m_pushDepth;
   NewLine();
}

void JsonPrettyPrinter::TabOut()
{
   for (auto i = 0u; i < m_pushDepth; ++i)
   {
      m_output << g_tabString;
   }
}

namespace
{
   bool IsPreformatState(JSONParseState state)
   {
      switch (state)
      {
      case JSONParseState::InNameString:
      case JSONParseState::NameStringEscape:
      case JSONParseState::InValueString:
      case JSONParseState::ValueStringEscape:
         return true;
      default: return false;
      }
   }
}

void JsonPrettyPrinter::SideEffectsBeforeChar(JSONParseSymbol symbol, JSONParseState currentState)
{
   if (!IsPreformatState(currentState) && m_enabled)
   {
      switch (symbol)
      {
      case JSONParseSymbol::ObjectEnd:
      case JSONParseSymbol::ListEnd:
         EndBlockBefore();
         break;
      default: break;
      }
   }
}
void JsonPrettyPrinter::SideEffectsAfterChar(JSONParseSymbol symbol, JSONParseState currentState)
{
   if (!IsPreformatState(currentState) && m_enabled)
   {
      switch (symbol)
      {
      case JSONParseSymbol::Separator:
         NewLine();
         break;
      case JSONParseSymbol::ListStart:
      case JSONParseSymbol::ObjectStart:
         StartBlock();
         break;
      case JSONParseSymbol::DelaratorEnd:
         InsertSpace();
         break;
      default: break;
      }
   }
}

void JsonPrettyPrinter::WriteChar(const char toWrite)
{
   m_output << toWrite;
}

void JsonPrettyPrinter::ColourForStateAndWrite(char nextChar)
{
   using namespace VT100;

   switch(m_state)
   {
   case JSONParseState::InNameString:
   case JSONParseState::NameEnd:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::Blue).SetAttribute(DisplayAttribute::Bright).SetAttribute(DisplayAttribute::Underscore);
      WriteChar(nextChar);
   break;
   case JSONParseState::ConsumeNumeric:
   case JSONParseState::ConsumeFloat:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::Green);
      WriteChar(nextChar);
      break;
   case JSONParseState::InValueString:
   case JSONParseState::ValueStringEnd:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::Cyan);
      WriteChar(nextChar);
      break;
   case JSONParseState::NameStringEscape:
   case JSONParseState::ValueStringEscape:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::Red).SetAttribute(DisplayAttribute::Blink);
      WriteChar(nextChar);
      break;
case JSONParseState::ConsumeBool:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::Magenta);
      WriteChar(nextChar);
      break;
   default:
      SetDisplay(m_output).SetAttribute(DisplayAttribute::Reset).SetColour(ColourIndex::White);
      WriteChar(nextChar);
   }

}

JsonPrettyPrinter::JSONParseState JsonPrettyPrinter::GetNextState(JSONParseSymbol symbol)
{
   auto nextState = JsonPrettyPrinter::g_transitions[underlying_cast(m_state)][underlying_cast(symbol)];

   //It's better to just carry on than to fail completely in this case
   if (nextState == JSONParseState::MAX)
   {
      SetDisplay(m_output).SetColour(VT100::ColourIndex::Red).SetBackgroundColour(VT100::ColourIndex::Yellow); //Set some loud formatting to draw attention
      nextState = m_state;
   }   

   return nextState;
}

void JsonPrettyPrinter::ConsumeChar(char character)
{
   auto symbol = Classify(character);
   auto nextState = GetNextState(symbol);
   auto prevState = m_state;
   
   SideEffectsBeforeChar(symbol, m_state);
   
   if (nextState != m_state)
   {
      m_state = nextState;
      ColourForStateAndWrite(character);
   }  
   else
   {
      WriteChar(character);  
   }
   SideEffectsAfterChar(symbol, prevState);
}

void JsonPrettyPrinter::Reset()
{
   VT100::SetDisplay(m_output).SetDefault();
}

JsonPrettyPrinter::JSONParseSymbol JsonPrettyPrinter::Classify(char character)
{
   if (  (character >= 'a' && character <= 'z')
      || (character >= 'Z' && character <= 'Z') )
   {
      return JSONParseSymbol::Alphabetic;
   }
   else if (character >= '0' && character <= '9')
   {
      return   JSONParseSymbol::Numeric;
   }

   switch (character)
   {
   case '\t':  //fallthrough
   case '\n':  //fallthrough
   case ' ':   return JSONParseSymbol::WhiteSpace;
   case '-':   //fallthrough 
   case '+':   return JSONParseSymbol::Signedness;
   case '.':   return JSONParseSymbol::DecimalPoint;
   case ':':   return JSONParseSymbol::DelaratorEnd;
   case '"':   return JSONParseSymbol::DoubleQuote;
   case '\\':  return JSONParseSymbol::Escape;
   case ']':   return JSONParseSymbol::ListEnd;
   case '[':   return JSONParseSymbol::ListStart;
   case '}':   return JSONParseSymbol::ObjectEnd;
   case '{':   return JSONParseSymbol::ObjectStart;
   case ',':   return JSONParseSymbol::Separator;
   default:    return JSONParseSymbol::OtherPrintableSymbol;
   }
}

void JsonPrettyPrinter::Print(const std::string& json)
{
   for (const auto &c : json)
   {
      ConsumeChar(c);
   }
}