#if _MODO_COMPETICAO_
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <stdio.h>
#include <tchar.h>


#include "MeuAgentePrincipal.h"
#include <Unidade.h>
#include <AgentePrincipal.h>
#include <Gerente.h>

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	return AgentePrincipal::DllMain(ul_reason_for_call);
}

 extern "C" __declspec(dllexport) void* newAIModule(void* game)
{
	printf("Competition...\n");
	agentePrincipal = new MeuAgentePrincipal();
	return AgentePrincipal::NewAiModule(game);
}

#elif _MODO_TESTE_

#include <windows.h>
#include <stdio.h>
#include <tchar.h>


#include "MeuAgentePrincipal.h"
#include <Unidade.h>
#include <AgentePrincipal.h>
#include <Gerente.h>

int main(){
	agentePrincipal = new MeuAgentePrincipal();
	printf("Testing...\n");
	AgentePrincipal::IniciarEmModoTeste();	
	return 0;
}

#endif