#include <stdlib.h>
#include <string.h>

#include "match.h"


/********************************************************************/
/*                                                                  */
/*   FUNCTION:                                                      */
/*      short match(table,token)                                    */
/*                                                                  */
/*   DESCRIPTION:                                                   */
/*      Identifies the position in "table" that matches "token".    */
/*                                                                  */
/*   PARAMETERS:                                                    */
/*      table - pointer to pointers to command names.               */
/*              Must be terminated by a NULL pointer.               */
/*      token - pointer to incoming command.                        */
/*                                                                  */
/*                                                                  */
/*   EXIT:                                                          */
/*      Returns the integer position into "table", or NO_MATCH      */
/*      if not found.                                               */
/*                                                                  */
/*   EXTERNAL EFFECTS:                                              */
/*                                                                  */
/*   WARNINGS:                                                      */
/*                                                                  */
/*   ROUTINES CALLED:                                               */
/*                                                                  */
/********************************************************************/
short match(char *table[], char *token)
{
  short table_pos = 0;              /* Position in table             */
  short token_len;                  /* Length of token to match      */
  short abbrev = 0;                 /* Found an abbreviation flag    */
  short i;                          /* simple counter                */

  token_len = strlen(token);
  while (table[table_pos]){         /* As long as something is there */

    if ( strcmpi(table[table_pos],token) ){
                                    /* If no match; try for abbrev.  */
      for (i=0; (toupper(*(table[table_pos]+i))==toupper(token[i])) && i<token_len; ++i);
      if (i==token_len){            /* Is it an abbreviation?        */
        if (abbrev){                /* Yes, check if first           */
          abbrev = -1;              /* Already saw an abbreviation   */
        } else {
          abbrev = table_pos+1;     /* Save position+1               */
        } /* endif */
      } /* endif */
      table_pos += 1;               /*   then try next               */
    } else {
      return table_pos;             /* Eureka! we have it.           */
    } /* endif */
  } /* endwhile */
  if (abbrev>0){                    /* Did we see ONE abbreviation?  */
    return abbrev-1;                /* Yes, return the position.     */
  } /* endif */
  return NO_MATCH;                  /* Sorry, Charlie. Better luck
                                       next time                     */
}
