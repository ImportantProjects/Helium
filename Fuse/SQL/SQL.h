#pragma once

#include "API.h"
#include "Common/Assert.h"
#include "Common/Types.h"


namespace SQL
{
  typedef SQL_API i32 StmtHandle;
  const static i32 NullStatement = 0; 

  static const i64 InvalidRowID = -1;

  /////////////////////////////////////////////////////////////////////////////
  namespace BinaryOperators
  {
    enum BinaryOperator
    {
      Equal,
      NotEqual,
      Greater,
      GreaterOrEqual,
      Lesser,
      LesserOrEqual,
      Like,
      NotLike,
      And,
      Or,

      // Number of operators, not an actual operator
      Count,
    };

    static const std::string s_String[Count] = 
    {
      "=",
      "!=",
      ">",
      ">=",
      "<",
      "<=",
      "LIKE",
      "NOT LIKE",
      "AND",
      "OR",
    };

    inline const std::string& String( const u32 id )
    {
      NOC_ASSERT( id < Count );
      return s_String[id];
    }
  }
  SQL_API typedef BinaryOperators::BinaryOperator BinaryOperator;
}