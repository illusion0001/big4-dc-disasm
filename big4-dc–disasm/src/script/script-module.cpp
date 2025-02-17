/******************************************************************************/
/*
  Author  - icemesh
*/
/******************************************************************************/

#include "script-module.h"
#include "../dc/dc.h"
#include "../dc/state-script/state-script.h" //state script header
#include "../ss-debug/state-script/state-script-debug.h"
#include "../ss-debug/level-set/level-set-debug.h"
#include "../ss-debug/render-settings/render-settings-debug.h"
#include "../script/script-manager-eval.h"
#include "../dc/id-group/id-group.h"
#include "../sidbase/sidbase.h"

#include <stdio.h>
#include <intrin.h> //bittest
#include <cinttypes>
#include <stdlib.h>

Module::Module() : m_pLoadedFile(nullptr) {}

Module::Module(const char* filename)
{
	m_pLoadedFile = nullptr;
	FILE* fh;
	fopen_s(&fh, filename, "rb");
	if (fh)
	{
		fseek(fh, 0x0, SEEK_END);
		size_t fsize = ftell(fh);
		fseek(fh, 0x0, SEEK_SET);
		void* pFile = malloc(fsize);
		if (pFile)
		{
			fread(pFile, fsize, 0x1, fh);
			fclose(fh);
			m_pLoadedFile = pFile;
		}
		else
		{
			puts("Failed to allocate file");
			fclose(fh);
		}
	}
	else
	{
		printf("Could not open %s! - Does it even exist?\n", filename);
	}
}

Module::~Module()
{
	if (m_pLoadedFile != nullptr)
	{
		free(m_pLoadedFile);
	}
}

int32_t Module::Login()
{
	if (m_pLoadedFile)
	{
		std::uint8_t* pScript = reinterpret_cast<std::uint8_t*>(m_pLoadedFile);
		return ScriptLogin(pScript, pScript, 0x0, 0xFFFFFFFFFFFFFFFF);
	}
	else
	{
		puts("Invalid file");
		return 0;
	}
}

//this can be improved... Am i going to ? probably not :^)
int32_t Module::ScriptLogin(uint8_t* pFile, uint8_t* pFile1, uint64_t lowerBound, uint64_t upperBound)
{
	//mov     rsi, [rdi]
	uint64_t magic = *(uint64_t*)pFile;
	//cmp     esi, 44433030
	if ((uint32_t)magic == 0x44433030)
	{
		if ((magic >> 0x20) == 1)
		{
			//movsxd  r10, dword ptr [rdi+8]
			int32_t v0 = *(int32_t*)(pFile + 0x8);
			//mov     bl, 1

			//mov     r11d, [rdi+r10]
			int32_t size = *(int32_t*)(pFile + v0);
			//mov     [rbp+size], r11

			//test    r11d, 1FFFFFFFh
			if (size & 0x1FFFFFFF)
			{
				//add     r10, rdi
				uint8_t* pUnk = pFile + v0;
				//lea     r8d, ds:0[r11*8] ; Load Effective Address
				uint32_t max = size * 8;
				//xor     r14d, r14d      ; iPtr = 0
				uint32_t iPtr = 0;
				//xor     r13d, r13d      ; Logical Exclusive OR
				int32_t byteIndex = 0;
				//xor     ebx, ebx        ; Logical Exclusive OR
				int32_t v1 = 0;
				//mov     [rbp+lowerBound], rdx
				//mov     [rbp+pFile_and_r10], r10
				//mov     [rbp+Max], r8
				int8_t r12d = 0x0;
				//eax
				int32_t v2 = 0x0;

				int32_t r15d = 0x0;

				uint64_t loc = 0;
				uint64_t tmp = 0;
				do {
					if (byteIndex < size)
					{
						//lea     r12d, [rbx+1]   ; Load Effective Address
						r12d = v1 + 1;
						//mov     esi, 0

						//mov     eax, r13d
						//movzx   eax, byte ptr [r10+rax+4] ; Move with Zero-Extend
						v2 = *(pUnk + byteIndex + 0x4);
						//cmp     r12d, 8         ; Compare Two Operands
						if (r12d == 8)
						{
							//setz    dl              ; Set Byte if Zero (ZF=1)
							r15d = 1;
							//cmovz   r12d, esi       ; Move if Zero (ZF=1)
							r12d = 0;
						}
						else
						{
							//movzx   r15d, dl        ; Move with Zero-Extend
							r15d = 0;
						}
						//bt      eax, ebx 
						if (_bittest(reinterpret_cast<long*>(&v2), v1))
						{
							//mov     rdx, [rdi+r14*8] ; loc = *(uint64_t*)(pFile+iPtr+0x8)
							loc = *(uint64_t*)(pFile + (iPtr * 0x8));//iPtr*sizeof(void*) platform dependant
							//mov     eax, 0
							tmp = 0;
							if (loc != 0)
							{
								if ((loc >= lowerBound && loc < upperBound))
								{
									//mov     rax, rdx
									tmp = loc;
								}
							}
							//add     rax, r9
							//mov     [rdi+r14*8], rax
							*(uint64_t*)(pFile + (iPtr * 0x8)) = (uint64_t)(pFile1 + tmp);
						}
						//add     r13d, r15d
						byteIndex += r15d;
						//inc     r14
						iPtr++;
						//mov     ebx, r12d
						v1 = r12d;
					}
					//cmp     r8d, r14d
				} while (max > iPtr);
				return 1;
			}
			else
			{
				return 1;
			}
		}
		else
		{
			printf("Error! Wrong version number. (%d/%lld)\n", 1, (magic >> 0x20));
			return 0;
		}
	}
	else
	{
		printf("Error! Invalid magic number. (%08X/%016llX)\n", 0x44433030, (magic & 0xFFFFFFFF));
		return 0;
	}
}

void Module::DumpEntry(Entry* pEntry)
{
	StringId scriptType = pEntry->m_typeId;
	switch (scriptType)
	{
		case 0x45C51F9F:
		{
			DumpScript(reinterpret_cast<StateScript*>(pEntry->m_entryPtr));
			break;
		}

		case 0xC7CB275C: //SID("int32")
		{
			int32_t* pVal = reinterpret_cast<int32_t*>(pEntry->m_entryPtr);
			printf("    int32 '%s = %d\n", StringIdToStringInternal(pEntry->m_scriptId), *pVal);
			break;
		}

		case 0xC4AB6121: //SID("symbol")
		{
			StringId* pVal = reinterpret_cast<StringId*>(pEntry->m_entryPtr);
			printf("    symbol '%s = '%s\n", StringIdToStringInternal(pEntry->m_scriptId), StringIdToStringInternal(*pVal) );
			break;
		}

		case 0x98D57064: //SID("level-set")
		{
			PrintLevelSet(reinterpret_cast<LevelSet*>(pEntry->m_entryPtr));
			break;
		}

		case 0x0BE3B01C:
		{
			PrintRenderSettings(reinterpret_cast<RenderSettingsMap*>(pEntry->m_entryPtr));
			break;
		}

		case 0x9ED499E1: //SID("script-lambda")
		{
			printf("(defun '%s\n", StringIdToStringInternal(pEntry->m_scriptId));
			ExecuteScriptCode(reinterpret_cast<ScriptLambda*>(pEntry->m_entryPtr));
			puts(")");
			break;
		}

		case 0x0372CF7E: //SID("id-group")
		{
			printf("(define-id-group '%s\n", StringIdToStringInternal(pEntry->m_scriptId));
			IdGroup* pGroup = reinterpret_cast<IdGroup*>(pEntry->m_entryPtr);
			int64_t numEntries = pGroup->m_numEntries;
			IdGroupEntry* pEntry = pGroup->m_pEntries;
			for (int64_t iEntries = 0; iEntries < numEntries; iEntries++)
			{
				printf("    :name '%s\n", StringIdToStringInternal(pEntry->m_id));
				pEntry++;
			}
			puts(")");
			break;
		}

		default:
		{
			printf( "Unk case:\n"
				"EntryId: %s\n"
				"    type:  %s(0x%08X)\n",
				StringIdToStringInternal(pEntry->m_scriptId),
				StringIdToStringInternal(scriptType),
				scriptType);
			break;
		}
	}
	puts("");
}
