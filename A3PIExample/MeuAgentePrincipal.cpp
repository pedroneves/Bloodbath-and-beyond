#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
<<<<<<< HEAD
#include <math.h> 
=======
>>>>>>> 5430eeee0b2cf3099ebe0d935bc1f58af1c4a62d
using namespace BWAPI;

//blackboard!
BWAPI::Position centro;
BWAPI::Position base;
BWAPI::Position base_inimiga;



Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

Unidade* scout;


//RECURSOS//
Unidade* Protoss_Gateways [10];
Unidade* Protoss_Pylons [10];
Unidade* Protoss_Workers [10];
int numWorkers;
int numGateways;
int numPylons;
bool resourceSemaphore;


bool GameOver = false;
bool hasScout = false;
Unidade* amigoDaVez = NULL;
//


//RECURSOS, PODE SER UTIL PARA MILITAR TAMBEM 
void updateGateways(){
	int cont = 0;
	std::set<Unidade*> unidades= Protoss_Nexus->getAllyUnits();
	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++)
	{
		if ((*it)->getType() == BWAPI::UnitTypes::Protoss_Gateway)
		{
			Protoss_Gateways [cont] = (*it);
			cont++;
		}
	}
	numGateways = cont;
}

void updatePylons(){
	int cont = 0;
	std::set<Unidade*> unidades= Protoss_Nexus->getAllyUnits();
	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++)
	{
		if ((*it)->getType() == BWAPI::UnitTypes::Protoss_Pylon)
		{
			Protoss_Pylons [cont] = (*it);
			cont++;
		}
	}
	numPylons = cont;
}

void updateWorkers(){
	int cont = 0;
	std::set<Unidade*> unidades= Protoss_Nexus->getAllyUnits();
	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++)
	{
		if ((*it)->getType().isWorker())
		{
			Protoss_Workers [cont] = (*it);
			cont++;
		}
	}
	numWorkers = cont;
}

void AITrabalhador (Unidade* u){
	double distance = 99999;
	Unidade* mineralPerto = NULL;
	std::set<Unidade*> minerais = u->getMinerals();
	for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++){
		if(u->getDistance(*it) < distance){
			distance = u->getDistance(*it);
			mineralPerto = *it;
		}
	}
	if(mineralPerto != NULL) u->rightClick(mineralPerto);
	if(mineralPerto == NULL){
		u->move(Protoss_Nexus->getPosition());
	}
}


void AIBatedor (Unidade* u){

    /*
		A primeira etapa do para o scout seria dividir o mapa em quadrantes.

		Geralmente, os combates em SC, os jogadores inimigos se localizam em quadrantes opostos
    */

	BWAPI::Position positionU = u->getPosition();
	double opostoX = 0;
	double opostoY = 0;

	bool once = true;

	if(positionU.x() < centro.x()){
		opostoX = (centro.x() - positionU.x()) + centro.x();
	}else{
		opostoX = centro.x() - (positionU.x() - centro.x());
	}

	if(positionU.y() < centro.y()){
		opostoY = (centro.y() - positionU.y()) + centro.y();
	}else{
		opostoY = centro.y() - (positionU.y() - centro.y());
	}

	if(once){
		u->move(BWAPI::Position(opostoX, opostoY));
		once = false;
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
<<<<<<< HEAD
			if(u == batedor){AIBatedor(u);}
			else if(u->getType().isWorker()){AITrabalhador(u);}
=======
			if(u == amigoDaVez) AIBatedor(u);
			else if(u == scout) AIBatedor(u);
			else if(u->getType().isWorker()) AITrabalhador(u);
>>>>>>> 5430eeee0b2cf3099ebe0d935bc1f58af1c4a62d
			else {caboSoldado[0] = AIGuerreiro(caboSoldado);}
		}
		//
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
		updateWorkers();
		updatePylons();
		updateGateways();

		if(Protoss_Nexus->minerals() > 50 && numWorkers < 6 && !Protoss_Nexus->isTraining()){
			Protoss_Nexus->train(BWAPI::UnitTypes::Protoss_Probe);		
		}

		//Construindo pylons sempre no angulo mais longe dos recursos! Se o inimigo tiver interesse em atacar estruturas, os trabalhadores estarao seguros!
		if(Protoss_Nexus->minerals() > 100 && ((numWorkers == 6 && numPylons==0) || (Protoss_Nexus->supplyTotal()-Protoss_Nexus->supplyUsed())<3) && resourceSemaphore == false){
			resourceSemaphore = true;
			Unidade* worker = Protoss_Workers[numWorkers-1];
			Unidade* closestMin = NULL;
			double distance = 99999;
			std::set<Unidade*> minerais = worker->getMinerals();
			for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++){
				if(worker->getDistance(*it) < distance){
					distance = worker->getDistance(*it);
					closestMin = *it;
				}
			}
			Unidade* nexus = Protoss_Nexus;
			double delta_y;
			double delta_x;
			if(closestMin != NULL){
				delta_y	= nexus->getPosition().y() - closestMin->getPosition().y();
				delta_x =  nexus->getPosition().x() - closestMin->getPosition().x();
				double angle = atan2(delta_y, delta_x) * 180.0 / 3.14159265;
				BWAPI::Position setPos = BWAPI::Position(nexus->getPosition().x()+300*cos(angle),nexus->getPosition().y()+300*sin(angle));
				while(!setPos.isValid()){
					int distance = 30*(rand()%10);
					setPos = BWAPI::Position(nexus->getPosition().x()+distance*cos(angle),nexus->getPosition().y()+distance*sin(angle));
				}
				worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
			}
			resourceSemaphore = false;
		}
		
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
		centro = BWAPI::Position((AgentePrincipal::mapWidth()*32), (AgentePrincipal::mapHeight()*32));
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


