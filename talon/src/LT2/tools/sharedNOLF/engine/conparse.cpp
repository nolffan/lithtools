#include "bdefs.h"
#include "ltbasedefs.h"
#include "conparse.h"



#define QUOTE_CHAR		'\"'
#define SPECIAL_CHAR	'%'



enum GNTResult
{
	GNT_NoToken=0,
	GNT_GotToken,
	GNT_GotSemicolon
};


// talon: NOLF's tokenizer (0x40dd60) is the older one Build 69's engine
// still has (nolf2src's runtime conparse.cpp): one loop with a paren count,
// '(' and '"' opening a group and ')' or '"' closing it. AvP2's, with a
// routine for each group, is later.
static GNTResult cp_GetNextToken(char* &pCurPos, char* &pTokenPos);



// ConParse implementation.
LTBOOL ConParse::Parse()
{
	if(!m_pCommandPos)
		return LTFALSE;

	if(!cp_Parse(m_pCommandPos, &m_pCommandPos, m_ArgBuffer, m_Args, &m_nArgs))
	{
		m_pCommandPos = LTNULL;
	}

	return LTTRUE;
}

LTBOOL ConParse::ParseFind(char *pLookFor, LTBOOL bCaseSensitive, uint32 minTokens)
{
	LTBOOL equal;

	// Must have at least one token, otherwise it can't find anything.
	if(minTokens == 0)
		minTokens = 1;

	while(Parse())
	{
		if(m_nArgs >= (int)minTokens)
		{
			if(bCaseSensitive)
				equal = (strcmp(m_Args[0], pLookFor) == 0);
			else
				equal = (stricmp(m_Args[0], pLookFor) == 0);

			if(equal)
			{
				return LTTRUE;
			}
		}
	}

	return LTFALSE;
}


// 0x40dee0
int cp_Parse(char *pCommand, char **pNewCommandPos, char *argBuffer, char **argPointers, int *nArgs)
{
	GNTResult status;
	char *pCurArgBufferPos, *pToken;
	char *pCurPos;


	// Parse.
	pCurPos = pCommand;
	pCurArgBufferPos = argBuffer;
	*nArgs = 0;
	
	for(;;)
	{
		pToken = pCurArgBufferPos;
		status = cp_GetNextToken(pCurPos, pCurArgBufferPos);

		if(status == GNT_NoToken)
		{
			// All done..
			return 0;
		}
		else if(status == GNT_GotToken)
		{
			argPointers[*nArgs] = pToken;
			
			++(*nArgs);
			if(*nArgs >= PARSE_MAXARGS)
				break;
		}
		else if(status == GNT_GotSemicolon)
		{
			// Got a semicolon.. finish up and tell them that there are more.
			*pNewCommandPos = pCurPos;
			return 1;
		}
	}

	return 0;
}


static inline LTBOOL cp_IsCmdSeparator(char curChar)
{
	return curChar == ';' || iscntrl(curChar);
}


static GNTResult cp_GetNextToken(char* &pCurPos, char* &pTokenPos)
{
	char *pToken;
	char curChar;
	LTBOOL parenCount, bEnd = LTFALSE;


	parenCount = 0;

	// Skip spaces.
	while(pCurPos[0] == ' ')
		pCurPos++;

	// Is there even a string?
	if(pCurPos[0] == 0)
		return GNT_NoToken;

	pToken = pTokenPos;
	for(;;)
	{
		// Get the char.
		curChar = *pCurPos;

		// End of string?
		if(curChar == 0)
		{
			bEnd = LTTRUE;
		}
		else if(curChar == SPECIAL_CHAR)
		{
			// Just add the next character to the string.
			pCurPos++;
			curChar = *pCurPos;
			if(curChar == 0)
			{
				bEnd = LTTRUE;
			}
			else
			{
				goto ADD_TO_OUTPUT_STRING;
			}
		}
		else if(cp_IsCmdSeparator(curChar))
		{
			// If this is the first character, then return the fact that it's a semicolon.
			*pTokenPos = 0;
			++pTokenPos;
			if(pToken[0] == 0)
			{
				// Only increment it if it's a full semicolon delimiter, so that
				// next time around parsing, it'll skip past the semicolon.
				++pCurPos;
				return GNT_GotSemicolon;
			}
			else
			{
				return GNT_GotToken;
			}
		}
		else if(curChar == '(')
		{
			parenCount++;

			if(parenCount == 1)
			{
				// If this was the first one, skip this character.
			}
			else
			{
				// Treat it like a normal character.
				goto ADD_TO_OUTPUT_STRING;
			}
		}
		else if(curChar == ')')
		{
			--parenCount;
			if(parenCount == 0)
			{
				// Ignore this character and end the current token.
				++pCurPos;
				bEnd = LTTRUE;
			}
			else
			{
				// Treat it like a normal character.
				goto ADD_TO_OUTPUT_STRING;
			}
		}
		else if(curChar == QUOTE_CHAR)
		{
			// Either starts a parenthesis or ends them all.
			if(parenCount)
			{
				++pCurPos;
				bEnd = LTTRUE;
			}
			else
			{
				parenCount = 1;
			}
		}
		else if(curChar == ' ' && parenCount == 0)
		{
			bEnd = LTTRUE;
		}
		else
		{
			ADD_TO_OUTPUT_STRING:;

			*pTokenPos = curChar;
			if((pTokenPos - pToken) >= PARSE_MAXARGLEN)
			{
				*pTokenPos = 0; // Just truncate it.
				--pTokenPos; // Decrement it so it doesn't overflow but it eats up the rest of the token.
			}

			++pTokenPos;
		}

		// Are we done?
		if(bEnd)
		{
			*pTokenPos = 0;
			++pTokenPos;
			if(pToken[0] == 0)
			{
				return GNT_NoToken;
			}
			else
			{
				return GNT_GotToken;
			}
		}

		++pCurPos;
	}
}
