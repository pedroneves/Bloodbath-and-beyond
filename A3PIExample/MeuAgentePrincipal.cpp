#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
using namespace BWAPI;
using namespace std;


//blackboard!
BWAPI::Position centro;
BWAPI::Position base;
BWAPI::Position base_inimiga;

Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

Unidade* scout;

bool GameOver = false;
bool hasScout = false;
Unidade* amigoDaVez = NULL;
//

void AITrabalhador (Unidade* u){
	double distance = 99999;
	Unidade* mineralPerto = NULL;
	std::set<Unidade*> minerais = u->getMinerals();
	for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++){
		if(u->getDistance(*it) < distance){
			distance = u->getDistance(*it);
			mineralPerto = *it;
		}//vai minerar no mineral mais perto
	}
	if(mineralPerto != NULL) u->rightClick(mineralPerto);
}

void AIConstrutora (Unidade* u){
	if(u->minerals() < 149) return; //por conveniencia alguns metodos de Player estao sendo providos diretamente para as unidades. Vide inicio de Unidade.h
	BWAPI::TilePosition tp = u->getTilePosition();
	int limite = 0;
	int adi = 6;
	//Construir algo em algum lugar
	while(!(u)->build(tp, u->minerals() > 449 ? BWAPI::UnitTypes::Protoss_Nexus : BWAPI::UnitTypes::Protoss_Pylon)){
		if(((u->minerals()&0xF)>>2) < 2) 
			tp = BWAPI::TilePosition(tp.x(), tp.y()+adi);
		else
			tp = BWAPI::TilePosition(tp.x()+adi, tp.y());
		tp.makeValid();
		limite++;
		if(limite > 50) break;
		adi = -adi + (adi > 0 ? -2 : +2);
	}
	amigoDaVez = NULL;//Bug aqui: amigoDaVez vai ser escolhido de novo antes mesmo do predio terminar...
}

char getSector (BWAPI::Position p){
	/*
		Dividindo o mapa em 9 setores:

			   (C1)   (C2)
			  A  |  B  |  C  
			----------------- (L1)
			  K  |  L  |  M  
			----------------- (L2)
			  X  |  Y  |  Z  

		A funcao recebe uma posicao e retorna em qual setor estah
	*/

	int width = centro.x()*2;
	int height = centro.y()*2;

	double C1 = width / 3;
	double C2 = (2*width) / 3;

	double L1 = height / 3;
	double L2 = (2*height) / 3;

	if(p.x() < C1 && p.y() < L1) return 'A';
	if(p.x() > C1 && p.x() < C2 && p.y() < L1) return 'B';
	if(p.x() > C2 && p.y() < L1) return 'C';

	if(p.x() < C1 && p.y() > L1 && p.y() < L2) return 'K';
	if(p.x() > C1 && p.x() < C2 && p.y() > L1 && p.y() < L2) return 'L';
	if(p.x() > C2 && p.y() > L1 && p.y() < L2) return 'M';

	if(p.x() < C1 && p.y() > L2) return 'X';
	if(p.x() > C1 && p.x() < C2 && p.y() > L2) return 'Y';
	if(p.x() > C2 && p.y() > L2) return 'Z';
}

char getOppositeSector (char s){
	if(s == 'A') return 'Z';
	if(s == 'B') return 'Y';
	if(s == 'C') return 'X';
	if(s == 'K') return 'M';
	if(s == 'L') return 'L';
	if(s == 'M') return 'K';
	if(s == 'X') return 'C';
	if(s == 'Y') return 'B';
	if(s == 'Z') return 'A';
}

BWAPI::Position getSectorCenter (char s){

	/*
		Dividindo o mapa em 9 setores:

			   (C1)   (C2)
			  A  |  B  |  C  
			----------------- (L1)
			  K  |  L  |  M  
			----------------- (L2)
			  X  |  Y  |  Z  

		A funcao recebe um setor e retorna o ponto central do setor
	*/

	int width = centro.x()*2;
	int height = centro.y()*2;

	double C1 = width / 3;
	double C2 = (2*width) / 3;

	double L1 = height / 3;
	double L2 = (2*height) / 3;

	double halfSectorWidth = (C1 / 2);
	double halfSectorHeight = (L1 / 2);

	if(s == 'A') return BWAPI::Position((int) halfSectorWidth, (int) halfSectorHeight);
	if(s == 'B') return BWAPI::Position((int) (C1 + halfSectorWidth), (int) halfSectorHeight);
	if(s == 'C') return BWAPI::Position((int) (C2 + halfSectorWidth), (int) halfSectorHeight);
	if(s == 'K') return BWAPI::Position((int) halfSectorWidth, (int) (L1 + halfSectorHeight));
	if(s == 'L') return BWAPI::Position((int) (C1 + halfSectorWidth), (int) (L1 + halfSectorHeight));
	if(s == 'M') return BWAPI::Position((int) (C2 + halfSectorWidth), (int) (L1 + halfSectorHeight));
	if(s == 'X') return BWAPI::Position((int) halfSectorWidth, (int) (L2 + halfSectorHeight));
	if(s == 'Y') return BWAPI::Position((int) (C1 + halfSectorWidth), (int) (L2 + halfSectorHeight));
	if(s == 'Z') return BWAPI::Position((int) (C2 + halfSectorWidth), (int) (L2 + halfSectorHeight));
}

void AIBatedor (Unidade* u){

    /*
		A primeira etapa do para o scout seria dividir o mapa em quadrantes.

		Geralmente, os combates em SC, os jogadores inimigos se localizam em quadrantes opostos
    */

	BWAPI::Position positionU = u->getPosition();
	double opostoX = 0;
	double opostoY = 0;

	/*if(positionU.x() < centro.x()){
		opostoX = (centro.x() - positionU.x()) + centro.x();
	}else{
		opostoX = centro.x() - (positionU.x() - centro.x());
	}

	if(positionU.y() < centro.y()){
		opostoY = (centro.y() - positionU.y()) + centro.y();
	}else{
		opostoY = centro.y() - (positionU.y() - centro.y());
	}*/

	if(positionU.x() != centro.x() || positionU.y() != centro.y()){
		u->rightClick(centro);
	}
}

Unidade* AIGuerreiro (Unidade* caboSoldado[]){

	Unidade* cabo = caboSoldado[0];
	Unidade* self = caboSoldado[1];

	//self->attack(*(self->getEnemyUnits().begin())); //ataca uma unidade inimiga aleatoria. Assume que existe uma.
	//cuidado com bugs como este. O codigo acima daria crash de null pointer no exato momento que o time inimigo
	//nao possuisse mais unidades, antes da partida de fato acabar.

	return cabo;
}

DWORD WINAPI threadAgente(LPVOID param){
	
	Unidade* u = (Unidade*) param;
	Unidade *caboSoldado[2] = {NULL,u}; //caso seja soldado

	while(true){
		printf("%s\n", "hello world threadAgente");

		//Se houve algum problema (ex: o jogo foi fechado) ou a unidade estah morta, finalizar a thread
		if(GameOver || u == NULL || !u->exists()) return 0;
		//Enquanto a unidade ainda nao terminou de ser construida ou o seu comando ainda nao foi
		//processado (ou seja, continua no mesmo turno), aguardar e poupar processamento
		if(!u->isCompleted()){
			Sleep(500);
			continue;
		}
		if(!u->checkNovoTurno()){
			Sleep(10);
			continue;
		}
		//Inserir o codigo de voces a partir daqui//
		if(u->isIdle()){ //nao ta fazendo nada, fazer algo util
			if(u == amigoDaVez) AIConstrutora(u);
			else if(u == scout) AIBatedor(u);
			else if(u->getType().isWorker()) AITrabalhador(u);
			else {caboSoldado[0] = AIGuerreiro(caboSoldado);}
		}
		else if(u->getType().isWorker() && u == amigoDaVez) AIConstrutora(u); //construir msm q estivesse fazendo algo
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}

DWORD WINAPI general_militar(LPVOID param){

	while(true){
		//Se houve algum problema (ex: o jogo foi fechado) ou a unidade estah morta, finalizar a thread
		if(GameOver) return 0;
		
		if(Protoss_Nexus != NULL && Protoss_Nexus->exists() && !Protoss_Nexus->checkNovoTurno()){
			Sleep(10);
			continue;
		}
		//Inserir o codigo de voces a partir daqui//
		// Codigo General Militar









		
		//Fim Codigo Genaral Militar
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}

DWORD WINAPI general_recursos(LPVOID param){

	while(true){
		//Se houve algum problema (ex: o jogo foi fechado) ou a unidade estah morta, finalizar a thread
		if(GameOver) return 0;
		
		if(Protoss_Nexus != NULL && Protoss_Nexus->exists() && !Protoss_Nexus->checkNovoTurno()){
			Sleep(10);
			continue;
		}
		//Inserir o codigo de voces a partir daqui//
		// Codigo General Recursos









		
		//Fim Codigo Genaral recursos
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}


DWORD WINAPI general(LPVOID param){

	while(true){
		//Se houve algum problema (ex: o jogo foi fechado) ou a unidade estah morta, finalizar a thread
		if(GameOver) return 0;
		
		if(Protoss_Nexus != NULL && Protoss_Nexus->exists() && !Protoss_Nexus->checkNovoTurno()){
			Sleep(10);
			continue;
		}
		//Inserir o codigo de voces a partir daqui//
		// Codigo General


		//Protoss_Nexus->train(BWAPI::UnitTypes::Protoss_Probe);
		//Protoss_Gateway->train(BWAPI::UnitTypes::Protoss_Zealot);//se for um gateway
		if((Protoss_Nexus->supplyTotal() - Protoss_Nexus->supplyUsed() < 5 || Protoss_Nexus->minerals() > 449) && amigoDaVez == NULL){
			//botar no "blackboard" para alguem construir predios de supply
			std::set<Unidade*> amigos = Protoss_Nexus->getAllyUnits();
			for(std::set<Unidade*>::iterator it = amigos.begin(); it != amigos.end(); it++){
				if((*it)->getType().isWorker()){
					amigoDaVez = *it;
					break;
				}
			}
		}//Lembrar que ha varias threads rodando em paralelo. O erro presente neste metodo (qual?) nao resulta em crash do jogo, mas outros poderiam.







		
		//Fim Codigo Genaral
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}


void MeuAgentePrincipal::InicioDePartida(){
	//Inicializar estruturas de dados necessarias, ou outras rotinas de inicio do seu programa. Cuidado com concorrencia, 
	//em alguns casos pode ser recomendavel que seja feito antes do while na criacao de uma nova thread.
	GameOver = false;
}

void MeuAgentePrincipal::onEnd(bool isWinner){  
	//sinalizar e aguardar o tempo necessario para todas as threads se terminarem antes do jogo sair, evitando erros.
	GameOver = true;
	Sleep(550);
}

void MeuAgentePrincipal::UnidadeCriada(Unidade* unidade){
	//Uma nova unidade sua foi criada (isto inclui as do inicio da partida). Implemente aqui como quer tratar ela.
	BWAPI::UnitType tipo = unidade->getType();
	
	if(tipo == BWAPI::UnitTypes::Protoss_Nexus){
		
		Protoss_Nexus = unidade;
		base = Protoss_Nexus->getPosition();
		int halfWidth = (AgentePrincipal::mapWidth()*16);
		int halfHeight = (AgentePrincipal::mapHeight()*16);
		centro = BWAPI::Position(halfWidth, halfHeight);
		CreateThread(NULL,0,general,NULL,0,NULL);
		CreateThread(NULL,0,general_recursos,NULL,0,NULL);
		CreateThread(NULL,0,general_militar,NULL,0,NULL);

	}
	else if(tipo == BWAPI::UnitTypes::Protoss_Gateway){
	
		Protoss_Gateway = unidade;

	}
	//Nao desperdicar threads com predios que nao fazem nada
	else if(!tipo.canProduce()){
		if(tipo == BWAPI::UnitTypes::Protoss_Probe && !hasScout){
			hasScout = true;
			scout = unidade;
		}
		CreateThread(NULL,0,threadAgente,(void*)unidade,0,NULL);
	}
}

/*
	Os outros eventos nao existem numa arquitetura orientada a Agentes Autonomos, pois eram relacionados ao Player do Broodwar
	de maneira generica, nao sendo especificamente ligados a alguma unidade do jogador. Se desejado, seus comportamentos podem
	ser simulados através de técnicas ou estruturas de comunicação dos agentes, como por exemplo Blackboards.
*/


