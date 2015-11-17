#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
#include <math.h> 
using namespace BWAPI;

//blackboard!
BWAPI::Position base;
BWAPI::Position base_inimiga;



Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

Unidade* batedor;


//RECURSOS//
Unidade* Protoss_Gateways [10];
Unidade* Protoss_Pylons [10];
Unidade* Protoss_Workers [10];
Unidade* Selected_Worker;
int numWorkers;
int numGateways;
int numPylons;
bool resourceSemaphore;
bool pylonBuildingSemaphore;



bool GameOver = false;

//

//Modificado.
void AITrabalhador (Unidade* u){
	double distance = 99999;
	Unidade* mineralPerto = NULL;
	if(Protoss_Nexus != NULL){
	std::set<Unidade*> minerais = Protoss_Nexus->getMinerals();
	for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++){
		if(Protoss_Nexus->getDistance(*it) < distance){
			distance = Protoss_Nexus->getDistance(*it);
			mineralPerto = *it;
		}
	}
	if(mineralPerto != NULL){u->rightClick(mineralPerto);}
	if(mineralPerto == NULL){u->move(Protoss_Nexus->getPosition());}
	}
}
//Modificado

void AIBatedor (Unidade* u){

	int heig = AgentePrincipal::mapHeight();
    int wid = AgentePrincipal::mapWidth();
	if(AgentePrincipal::isWalkable(wid, heig)){
	
		u->move(BWAPI::Position(wid,heig));
	
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
			if(u == batedor){AIBatedor(u);}
			else if(u->getType().isWorker() && u!=Selected_Worker){AITrabalhador(u);}
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




//----------------------------INI RECURSOS---------------------------//
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
	{3
		if ((*it)->getType().isWorker())
		{
			Protoss_Workers [cont] = (*it);
			cont++;
		}
	}
	numWorkers = cont;
}

Position getBuildPos(BWAPI::Position target){
	//////////////////////////////////////////////////////////
	// Se a posicao atual estiver ocupada, passa para a proxima
	////////////////////////////////////////////////////////////
	BWAPI::Position received = target;
	
	while(!target.isValid()){
		printf("Bad position");
	}
	return received;
}

void buildPylon(){
	resourceSemaphore = true;
	Unidade* worker = Protoss_Workers[numWorkers-1];
	Selected_Worker = worker;
	Unidade* closestMin = NULL;
	int distance = 99999;
	std::set<Unidade*> minerais = Protoss_Nexus->getMinerals();
	for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++){
		if(Protoss_Nexus->getDistance(*it) < distance){
			distance = Protoss_Nexus->getDistance(*it);
			closestMin = *it;
			}
	}
	Unidade* nexus = Protoss_Nexus;
	int delta_y;
	int delta_x;
	int desviox= 0;
	int desvioy= 0;
	if(closestMin != NULL){
		delta_y	=	nexus->getPosition().y() - closestMin->getPosition().y();
		delta_x =	nexus->getPosition().x() - closestMin->getPosition().x();
		BWAPI::Position setPos = BWAPI::Position(nexus->getPosition().x()+delta_x*1.8/3,nexus->getPosition().y()+delta_y*1.8/3);
		worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
		while(!worker->isConstructing())
		{
			desviox += 5*(rand()%3-1);
			desvioy += 5*(rand()%3-1);
			printf("\nBAD pylon POS >>:%d Y:%d ",setPos.x(),setPos.y());
			setPos = BWAPI::Position(nexus->getPosition().x()+delta_x*2/3 +desviox,nexus->getPosition().y()+delta_y*2/3 +desvioy);
			worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
		}
		printf("\n\nX:%d Y:%d \n",setPos.x(),setPos.y());
		worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
	}else{
		Unidade* pylon_vizinho = Protoss_Pylons[numPylons-1];
		BWAPI::Position setPos = BWAPI::Position(nexus->getPosition().x()+50,nexus->getPosition().y()+50);
		worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
		while(!worker->isConstructing())
		{
			desviox += 5*(rand()%3-1);
			desvioy += 5*(rand()%3-1);
			printf("\nBAD POS >>:%d Y:%d ",setPos.x(),setPos.y());
			setPos = BWAPI::Position(nexus->getPosition().x()+delta_x*2/3 +desviox,nexus->getPosition().y()+delta_y*2/3 +desvioy);
			worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
		}
		worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Pylon);
	}
	Selected_Worker = NULL;
	resourceSemaphore = false;
}

void buildGateway(){
	resourceSemaphore = true;
	Unidade* worker = Protoss_Workers[numWorkers-1];
	Selected_Worker = worker;
	Unidade* Selected_Pylon = Protoss_Pylons[numPylons-1];
	Unidade* nexus = Protoss_Nexus;
	int delta_y;
	int delta_x;
	int desviox=0;
	int desvioy=0;
	delta_y	=	nexus->getPosition().y() - Selected_Pylon->getPosition().y();
	delta_x =	nexus->getPosition().x() - Selected_Pylon->getPosition().x();
	BWAPI::Position setPos = BWAPI::Position(Selected_Pylon->getPosition().x()-delta_x*3/2,Selected_Pylon->getPosition().y()-delta_y*3/2);
	worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Gateway);
	while(!worker->isConstructing())
	{
		desviox += 5*(rand()%3-1);
		desvioy += 5*(rand()%3-1);
		setPos = BWAPI::Position(Selected_Pylon->getPosition().x()-delta_x*3/2+desviox,Selected_Pylon->getPosition().y()-delta_y*3/2+desvioy);
		worker->build(BWAPI::TilePosition(setPos),UnitTypes::Protoss_Gateway);
	}
	printf("X:%d Y:%d \n",setPos.x(),setPos.y());
	Selected_Worker = NULL;
	resourceSemaphore = false;
}

Unidade* getIdleGateway(){
	Unidade* selectedGateway = NULL;
	for(int a = 0; a< numGateways-1; a++){
		if(!Protoss_Gateways[a]->isTraining() || Protoss_Gateways[a]->isCompleted()){
			selectedGateway = Protoss_Gateways[a];
			return selectedGateway;
		}
	}
}

void trainZealot(){
	Unidade* selectGateway = Protoss_Gateways[rand()%numGateways];
	if(!selectGateway->isTraining()){
		selectGateway->train(UnitTypes::Protoss_Zealot);
	}
}

int avaiableSupply(){
	return (Protoss_Nexus->supplyTotal()-Protoss_Nexus->supplyUsed());
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
		//updatePylons();
		//updateGateways();

		if(Protoss_Nexus->minerals() > 50 && numWorkers < 6 && !Protoss_Nexus->isTraining()){
			Protoss_Nexus->train(BWAPI::UnitTypes::Protoss_Probe);		
		}

		if(Protoss_Nexus->minerals() > 100 && ((numWorkers == 6 && numPylons==0) || (avaiableSupply()<3)) && resourceSemaphore == false){
			buildPylon();
		}

		if(Protoss_Nexus->minerals() > 150 && numPylons>0 && resourceSemaphore == false){
			buildGateway();
		}

		if(Protoss_Nexus->minerals() > 100 && avaiableSupply()>=2 && numGateways>0){
			trainZealot();
		}
		
		//Fim Codigo Genaral recursos
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}
//----------------------------FIM RECURSOS---------------------------//

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
		//if((Protoss_Nexus->supplyTotal() - Protoss_Nexus->supplyUsed() < 5 || Protoss_Nexus->minerals() > 449) && amigoDaVez == NULL){
			//botar no "blackboard" para alguem construir predios de supply
			/*std::set<Unidade*> amigos = Protoss_Nexus->getAllyUnits();
			for(std::set<Unidade*>::iterator it = amigos.begin(); it != amigos.end(); it++){
				if((*it)->getType().isWorker()){
					amigoDaVez = *it;
					break;
				}
			}*/
			//ISSO TALVEZ ESTEJA ATRAPALHANDO A CONSTRUCAO - fmm4

		//}//Lembrar que ha varias threads rodando em paralelo. O erro presente neste metodo (qual?) nao resulta em crash do jogo, mas outros poderiam.







		
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
		CreateThread(NULL,0,general,NULL,0,NULL);
		CreateThread(NULL,0,general_recursos,NULL,0,NULL);
		CreateThread(NULL,0,general_militar,NULL,0,NULL);
	}
	else if(tipo == BWAPI::UnitTypes::Protoss_Gateway){
		Protoss_Gateways[numGateways] = unidade;
		numGateways = numGateways+1;
	}else if(tipo == BWAPI::UnitTypes::Protoss_Pylon){
		Protoss_Pylons[numPylons] = unidade;
		numPylons = numPylons+1;
	}
	//Nao desperdicar threads com predios que nao fazem nada
	else if(!tipo.canProduce()){
		CreateThread(NULL,0,threadAgente,(void*)unidade,0,NULL);
	}
}

/*
	Os outros eventos nao existem numa arquitetura orientada a Agentes Autonomos, pois eram relacionados ao Player do Broodwar
	de maneira generica, nao sendo especificamente ligados a alguma unidade do jogador. Se desejado, seus comportamentos podem
	ser simulados através de técnicas ou estruturas de comunicação dos agentes, como por exemplo Blackboards.
*/


