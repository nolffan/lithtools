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


// talon: the 2001 tokenizer (0x40a2b0..), written from the shipped code:
// a parenthesized or quoted token is read by its own routine, parentheses
// nest, and an empty one is a single space.
static GNTResult cp_GetNextToken(char* &pCurPos, char* &pTokenPos);
static void cp_AddChar(char* &pTokenPos, char *pToken, char theChar);
static LTBOOL cp_ParseParens(char* &pCurPos, char* &pTokenPos, char *pToken);
static LTBOOL cp_ParseQuotes(char* &pCurPos, char* &pTokenPos, char *pToken);



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
	LTBOOL bEnd = LTFALSE;


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
				cp_AddChar(pTokenPos, pToken, curChar);
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
			// A whole token in parentheses.
			if(pTokenPos == pToken)
			{
				++pCurPos;
				cp_ParseParens(pCurPos, pTokenPos, pToken);
			}

			bEnd = LTTRUE;
		}
		else if(curChar == QUOTE_CHAR)
		{
			// A whole token in quotes.
			if(pTokenPos == pToken)
			{
				++pCurPos;
				cp_ParseQuotes(pCurPos, pTokenPos, pToken);
			}

			bEnd = LTTRUE;
		}
		else if(curChar == ' ')
		{
			bEnd = LTTRUE;
		}
		else
		{
			cp_AddChar(pTokenPos, pToken, curChar);
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


static void cp_AddChar(char* &pTokenPos, char *pToken, char theChar)
{
	*pTokenPos = theChar;
	if((pTokenPos - pToken) >= PARSE_MAXARGLEN)
	{
		*pTokenPos = 0; // Just truncate it.
		--pTokenPos; // Decrement it so it doesn't overflow but it eats up the rest of the token.
	}

	++pTokenPos;
}


// Reads up to the matching ')'.
static LTBOOL cp_ParseParens(char* &pCurPos, char* &pTokenPos, char *pToken)
{
	while(*pCurPos != 0 && *pCurPos != ')')
	{
		if(*pCurPos == SPECIAL_CHAR)
		{
			pCurPos++;
			cp_AddChar(pTokenPos, pToken, SPECIAL_CHAR);
			if(*pCurPos != 0)
				cp_AddChar(pTokenPos, pToken, *pCurPos++);
		}
		else if(*pCurPos == '(')
		{
			pCurPos++;
			cp_AddChar(pTokenPos, pToken, '(');
			cp_ParseParens(pCurPos, pTokenPos, pToken);
			cp_AddChar(pTokenPos, pToken, ')');
		}
		else
		{
			cp_AddChar(pTokenPos, pToken, *pCurPos++);
		}
	}

	if(*pCurPos == ')')
	{
		pCurPos++;
		if(pTokenPos == pToken)
			cp_AddChar(pTokenPos, pToken, ' ');

		return LTTRUE;
	}

	return LTFALSE;
}


// Reads up to the closing quote.
static LTBOOL cp_ParseQuotes(char* &pCurPos, char* &pTokenPos, char *pToken)
{
	while(*pCurPos != 0 && *pCurPos != QUOTE_CHAR)
	{
		if(*pCurPos == SPECIAL_CHAR)
		{
			pCurPos++;
			cp_AddChar(pTokenPos, pToken, SPECIAL_CHAR);
			if(*pCurPos != 0)
				cp_AddChar(pTokenPos, pToken, *pCurPos++);
		}
		else
		{
			cp_AddChar(pTokenPos, pToken, *pCurPos++);
		}
	}

	if(*pCurPos == QUOTE_CHAR)
	{
		pCurPos++;
		if(pTokenPos == pToken)
			cp_AddChar(pTokenPos, pToken, ' ');

		return LTTRUE;
	}

	return LTFALSE;
}
