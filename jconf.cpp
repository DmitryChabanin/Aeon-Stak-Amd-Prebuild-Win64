/*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "jconf.h"
#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "jext.h"
#include "console.h"

using namespace rapidjson;

/*
 * This enum needs to match index in oConfigValues, otherwise we will get a runtime error
 */
enum configEnum { iGpuThreadNum, aGpuThreadsConf, iPlatformIdx,
	bTlsMode, bTlsSecureAlgo, sTlsFingerprint, sPoolAddr, sWalletAddr, sPoolPwd,
	iCallTimeout, iNetRetry, iGiveUpLimit, iVerboseLevel, iAutohashTime,
	sOutputFile, iHttpdPort, bPreferIpv4 };

struct configVal {
	configEnum iName;
	const char* sName;
	Type iType;
};

//Same order as in configEnum, as per comment above
configVal oConfigValues[] = {
	{ iGpuThreadNum, "gpu_thread_num", kNumberType },
	{ aGpuThreadsConf, "gpu_threads_conf", kArrayType },
	{ iPlatformIdx, "platform_index", kNumberType },
	{ bTlsMode, "use_tls", kTrueType },
	{ bTlsSecureAlgo, "tls_secure_algo", kTrueType },
	{ sTlsFingerprint, "tls_fingerprint", kStringType },
	{ sPoolAddr, "pool_address", kStringType },
	{ sWalletAddr, "wallet_address", kStringType },
	{ sPoolPwd, "pool_password", kStringType },
	{ iCallTimeout, "call_timeout", kNumberType },
	{ iNetRetry, "retry_time", kNumberType },
	{ iGiveUpLimit, "giveup_limit", kNumberType },
	{ iVerboseLevel, "verbose_level", kNumberType },
	{ iAutohashTime, "h_print_time", kNumberType },
	{ sOutputFile, "output_file", kStringType },
	{ iHttpdPort, "httpd_port", kNumberType },
	{ bPreferIpv4, "prefer_ipv4", kTrueType }
};

constexpr size_t iConfigCnt = (sizeof(oConfigValues)/sizeof(oConfigValues[0]));

inline bool checkType(Type have, Type want)
{
	if(want == have)
		return true;
	else if(want == kTrueType && have == kFalseType)
		return true;
	else if(want == kFalseType && have == kTrueType)
		return true;
	else
		return false;
}

struct jconf::opaque_private
{
	Document jsonDoc;
	const Value* configValues[iConfigCnt]; //Compile time constant

	opaque_private()
	{
	}
};

jconf* jconf::oInst = nullptr;

jconf::jconf()
{
	prv = new opaque_private();
}

bool jconf::GetThreadConfig(size_t id, thd_cfg &cfg)
{
	if(id >= prv->configValues[aGpuThreadsConf]->Size())
		return false;

	const Value& oThdConf = prv->configValues[aGpuThreadsConf]->GetArray()[id];

	if(!oThdConf.IsObject())
		return false;

	const Value *idx, *intensity, *w_size, *aff;
	idx = GetObjectMember(oThdConf, "index");
	intensity = GetObjectMember(oThdConf, "intensity");
	w_size = GetObjectMember(oThdConf, "worksize");
	aff = GetObjectMember(oThdConf, "affine_to_cpu");

	if(idx == nullptr || intensity == nullptr || w_size == nullptr || aff == nullptr)
		return false;

	if(!idx->IsUint64() || !intensity->IsUint64() || !w_size->IsUint64())
		return false;

	if(!aff->IsUint64() && !aff->IsBool())
		return false;

	cfg.index = idx->GetUint64();
	cfg.intensity = intensity->GetUint64();
	cfg.w_size = w_size->GetUint64();

	if(aff->IsNumber())
		cfg.cpu_aff = aff->GetInt64();
	else
		cfg.cpu_aff = -1;

	return true;
}

size_t jconf::GetPlatformIdx()
{
	return prv->configValues[iPlatformIdx]->GetUint64();
}

bool jconf::GetTlsSetting()
{
	return prv->configValues[bTlsMode]->GetBool();
}

bool jconf::TlsSecureAlgos()
{
	return prv->configValues[bTlsSecureAlgo]->GetBool();
}

const char* jconf::GetTlsFingerprint()
{
	return prv->configValues[sTlsFingerprint]->GetString();
}

const char* jconf::GetPoolAddress()
{
	return prv->configValues[sPoolAddr]->GetString();
}

const char* jconf::GetPoolPwd()
{
	return prv->configValues[sPoolPwd]->GetString();
}

const char* jconf::GetWalletAddress()
{
	return prv->configValues[sWalletAddr]->GetString();
}

bool jconf::PreferIpv4()
{
	return prv->configValues[bPreferIpv4]->GetBool();
}

size_t jconf::GetThreadCount()
{
	return prv->configValues[aGpuThreadsConf]->Size();
}

uint64_t jconf::GetCallTimeout()
{
	return prv->configValues[iCallTimeout]->GetUint64();
}

uint64_t jconf::GetNetRetry()
{
	return prv->configValues[iNetRetry]->GetUint64();
}

uint64_t jconf::GetGiveUpLimit()
{
	return prv->configValues[iGiveUpLimit]->GetUint64();
}

uint64_t jconf::GetVerboseLevel()
{
	return prv->configValues[iVerboseLevel]->GetUint64();
}

uint64_t jconf::GetAutohashTime()
{
	return prv->configValues[iAutohashTime]->GetUint64();
}

uint16_t jconf::GetHttpdPort()
{
	return prv->configValues[iHttpdPort]->GetUint();
}

const char* jconf::GetOutputFile()
{
	return prv->configValues[sOutputFile]->GetString();
}

bool jconf::check_cpu_features()
{
	constexpr int AESNI_BIT = 1 << 25;
	constexpr int SSE2_BIT = 1 << 26;

	int cpu_info[4];
#ifdef _WIN32
	__cpuid(cpu_info, 1);
#else
	__cpuid(1, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);
#endif

	bHaveAes = (cpu_info[2] & AESNI_BIT) != 0;
	return (cpu_info[3] & SSE2_BIT) != 0;
}

bool jconf::parse_config(const char* sFilename)
{
	FILE * pFile;
	char * buffer;
	size_t flen;

	if(!check_cpu_features())
	{
		printer::inst()->print_msg(L0, "CPU support of SSE2 is required.");
		return false;
	}

	pFile = fopen(sFilename, "rb");
	if (pFile == NULL)
	{
		printer::inst()->print_msg(L0, "Failed to open config file %s.", sFilename);
		return false;
	}

	fseek(pFile,0,SEEK_END);
	flen = ftell(pFile);
	rewind(pFile);

	if(flen >= 64*1024)
	{
		fclose(pFile);
		printer::inst()->print_msg(L0, "Oversized config file - %s.", sFilename);
		return false;
	}

	if(flen <= 16)
	{
		printer::inst()->print_msg(L0, "File is empty or too short - %s.", sFilename);
		return false;
	}

	buffer = (char*)malloc(flen + 3);
	if(fread(buffer+1, flen, 1, pFile) != 1)
	{
		free(buffer);
		fclose(pFile);
		printer::inst()->print_msg(L0, "Read error while reading %s.", sFilename);
		return false;
	}
	fclose(pFile);

	//Replace Unicode BOM with spaces - we always use UTF-8
	unsigned char* ubuffer = (unsigned char*)buffer;
	if(ubuffer[1] == 0xEF && ubuffer[2] == 0xBB && ubuffer[3] == 0xBF)
	{
		buffer[1] = ' ';
		buffer[2] = ' ';
		buffer[3] = ' ';
	}

	buffer[0] = '{';
	buffer[flen] = '}';
	buffer[flen + 1] = '\0';

	prv->jsonDoc.Parse<kParseCommentsFlag|kParseTrailingCommasFlag>(buffer, flen+2);
	free(buffer);

	if(prv->jsonDoc.HasParseError())
	{
		printer::inst()->print_msg(L0, "JSON config parse error(offset %llu): %s",
			int_port(prv->jsonDoc.GetErrorOffset()), GetParseError_En(prv->jsonDoc.GetParseError()));
		return false;
	}


	if(!prv->jsonDoc.IsObject())
	{ //This should never happen as we created the root ourselves
		printer::inst()->print_msg(L0, "Invalid config file. No root?\n");
		return false;
	}

	for(size_t i = 0; i < iConfigCnt; i++)
	{
		if(oConfigValues[i].iName != i)
		{
			printer::inst()->print_msg(L0, "Code error. oConfigValues are not in order.");
			return false;
		}

		prv->configValues[i] = GetObjectMember(prv->jsonDoc, oConfigValues[i].sName);

		if(prv->configValues[i] == nullptr)
		{
			printer::inst()->print_msg(L0, "Invalid config file. Missing value \"%s\".", oConfigValues[i].sName);
			return false;
		}

		if(!checkType(prv->configValues[i]->GetType(), oConfigValues[i].iType))
		{
			printer::inst()->print_msg(L0, "Invalid config file. Value \"%s\" has unexpected type.", oConfigValues[i].sName);
			return false;
		}
	}

	size_t n_thd = prv->configValues[aGpuThreadsConf]->Size();
	if(prv->configValues[iGpuThreadNum]->GetUint64() != n_thd)
	{
		printer::inst()->print_msg(L0,
			"Invalid config file. Your GPU config array has %llu members, while you want to use %llu threads.",
			int_port(n_thd), int_port(prv->configValues[iGpuThreadNum]->GetUint64()));
		return false;
	}

	thd_cfg c;
	for(size_t i=0; i < n_thd; i++)
	{
		if(!GetThreadConfig(i, c))
		{
			printer::inst()->print_msg(L0, "Thread %llu has invalid config.", int_port(i));
			return false;
		}
	}

	if(!prv->configValues[iCallTimeout]->IsUint64() ||
		!prv->configValues[iNetRetry]->IsUint64() ||
		!prv->configValues[iGiveUpLimit]->IsUint64())
	{
		printer::inst()->print_msg(L0,
			"Invalid config file. call_timeout, retry_time and giveup_limit need to be positive integers.");
		return false;
	}

	if(!prv->configValues[iVerboseLevel]->IsUint64() || !prv->configValues[iAutohashTime]->IsUint64())
	{
		printer::inst()->print_msg(L0,
			"Invalid config file. verbose_level and h_print_time need to be positive integers.");
		return false;
	}

	if(!prv->configValues[iHttpdPort]->IsUint() || prv->configValues[iHttpdPort]->GetUint() > 0xFFFF)
	{
		printer::inst()->print_msg(L0,
			"Invalid config file. httpd_port has to be in the range 0 to 65535.");
		return false;
	}

#ifdef CONF_NO_TLS
	if(prv->configValues[bTlsMode]->GetBool())
	{
		printer::inst()->print_msg(L0,
			"Invalid config file. TLS enabled while the application has been compiled without TLS support.");
		return false;
	}
#endif // CONF_NO_TLS

	printer::inst()->set_verbose_level(prv->configValues[iVerboseLevel]->GetUint64());
	return true;
}
