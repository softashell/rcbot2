#include "bot.h"
#include "bot_kv.h"
#include "bot_globals.h"

#include <cstring>

#include "rcbot/logging.h"

void CRCBotKeyValueList::parseFile(std::fstream& fp)
{
	char buffer[2* RCBOT_MAX_KV_LEN];
	char szKey[RCBOT_MAX_KV_LEN];
	char szValue[RCBOT_MAX_KV_LEN];

	size_t iLine = 0;

	// parse profile ini
	while (fp.getline(buffer, 255))
	{
		iLine++;

		if ( buffer[0] == '#' ) // skip comment
			continue;

		size_t iLen = std::strlen(buffer);

		if ( iLen == 0 )
			continue;

		if ( buffer[iLen-1] == '\n' )
			buffer[--iLen] = 0;

		if ( buffer[iLen-1] == '\r' )
			buffer[--iLen] = 0;

		bool bHaveKey = false;

		size_t iKi = 0;
		size_t iVi = 0;

		for ( unsigned int iCi = 0; iCi < iLen; iCi ++ )
		{
			// ignore spacing
			if ( buffer[iCi] == ' ' )
				continue;

			if ( !bHaveKey )
			{
				if ( buffer[iCi] == '=' )
				{
					bHaveKey = true;
					continue;
				}

				// parse key

				if ( iKi < RCBOT_MAX_KV_LEN )
					szKey[iKi++] = buffer[iCi];													
			}
			else if ( iVi < RCBOT_MAX_KV_LEN )
				szValue[iVi++] = buffer[iCi];
			else
				break;
		}      

		szKey[iKi] = 0;
		szValue[iVi] = 0;

		logger->Log(LogLevel::TRACE, "m_KVs.emplace_back(%s,%s)", szKey, szValue);

		m_KVs.emplace_back(new CRCBotKeyValue(szKey,szValue));

	}
}

CRCBotKeyValueList :: ~CRCBotKeyValueList()
{
	for (CRCBotKeyValue*& m_KV : m_KVs)
	{
		delete m_KV;
		m_KV = nullptr;
	}

	m_KVs.clear();
}

CRCBotKeyValue *CRCBotKeyValueList :: getKV ( const char *key ) const
{
	for (CRCBotKeyValue* const m_KV : m_KVs)
	{
		if ( FStrEq(m_KV->getKey(),key) )
			return m_KV;
	}

	return nullptr;
}

bool CRCBotKeyValueList :: getFloat ( const char *key, float *val ) const
{
	CRCBotKeyValue* pKV = getKV(key);

	if ( !pKV )
		return false;
	
	*val = static_cast<float>(std::atof(pKV->getValue()));

	return true;
}

	
bool CRCBotKeyValueList :: getInt ( const char *key, int *val ) const
{
	CRCBotKeyValue* pKV = getKV(key);

	if ( !pKV )
		return false;
	
	*val = std::atoi(pKV->getValue());

	return true;
}


bool CRCBotKeyValueList :: getString (const char* key, const char** val) const
{
	CRCBotKeyValue* pKV = getKV(key);

	if ( !pKV )
		return false;

	*val = pKV->getValue();

	return true;
}

CRCBotKeyValue :: CRCBotKeyValue ( const char *szKey, const char *szValue )
{
	std::strncpy(m_szKey,szKey,RCBOT_MAX_KV_LEN-1);
	m_szKey[RCBOT_MAX_KV_LEN-1] = 0;
	std::strncpy(m_szValue,szValue,RCBOT_MAX_KV_LEN-1);
	m_szValue[RCBOT_MAX_KV_LEN-1] = 0;
}