#pragma once

#include <Unidade.h>
#include <AgentePrincipal.h>
#include <vector>

#include <sstream>

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

class MeuAgentePrincipal:public AgentePrincipal{
public:
	
	virtual void InicioDePartida();

	virtual void onEnd(bool isWinner);

	virtual void UnidadeCriada(Unidade* unidade);

};
