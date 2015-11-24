#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
#include <stdio.h>
#include <math.h>
#include <ctime>

using namespace BWAPI;
 
//blackboard!
BWAPI::Position centro;
BWAPI::Position base;
BWAPI::Position base_inimiga;



Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

// Scout vars ////////////////////////////////////////////////////////////////////////////////////////////////////
Unidade* scout;


// Informa para qual setor o scout esta indo
char currentDestSector; 
// Raio de tolerancia para o centro do setor
int goalRadius = 50;
// Em quanto o raio de tolerancia vai aumentar
int goalRadiusDelta = 75;
// Verificador para informar a ultima distancia conhecida para o centro do setor
double lastDistanceToNextSector = 100000;
// Quantidade de vezes que se tentou chegar ao centro do setor
int nextSectorReachTryAmount = 0;
// Quantidade maxima de vezes que se pode tentar chegar ao setor
int maxAmountTryReachGoalRadius = 2;


// Posicao da primeira estrutura inimiga encontrada
BWAPI::Position firstEnemyStructureFoundPosition;
// Flag se encontrou alguma estrutura inigmiga
bool foundFirstEnemyStructure = false;
// Posicao do centro de comando inimigo
BWAPI::Position enemyCommandPosition;
// Flag se encontrou centro de comando inigmigo
bool foundEnemyCommand = false;


// Em qual volta esta, quando se anda em espiral
int spiralTurn = 1;
// Em qual setor da espiral esta
int spiralSector = 0;
// O quanto o raio vai aumentar, quando se termina uma volta
int spiralRadiusDelta = 35;
// Raio de tolerancia para se atingir o setor da espiral
int spiralGoalRadius = 5;
// Posicao do proximo setor da espiral
BWAPI::Position nextSpiralSectorPosition;
// A ultima distancia conhecida para o proximo setor da espiral
double lastDistanceToNextSpiralSector;
// Quantidade de vezes que se tentou chegar ao centro do setor da espiral
int nextSpiralSectorReachTryAmount = 0;
// Quantidade maxima de vezes que se pode tentar chegar ao setor da espiral
int maxAmountTryReachSpiralGoalRadius = 3;


// Quantidade em segundos que se vai checar a visao
double visionCheckInterval = 1;
// Controle para saber se deve checar ou nao
double nextVisionCheck = (std::clock() / ((double) CLOCKS_PER_SEC)) + visionCheckInterval;


// Primeiro worker inimigo encontrado
Unidade* nextEnemyWorker;
// Flag para saber se encontrou algum inimigo
bool hasNextEnemyWorker = false;


// Minerais encontrados enquanto procurava o inimigo
std::set<Unidade*> mineralsFoundByScout;

// END SCOUT VARS //////////////////////////////////////////////////////////////////////////////////////////////////


// Soldier vars ////////////////////////////////////////////////////////////////////////////////////////////////////

int enemyCommandGoalRadius = 50;
int zealotCount = 0;
bool hasEnoughSoldiers = false;

// Quantidade em segundos que se vai checar a visao
double soldierVisionCheckInterval = 1;
// Controle para saber se deve checar ou nao
double nextSoldierVisionCheck = (std::clock() / ((double) CLOCKS_PER_SEC)) + soldierVisionCheckInterval;
int zealotCountVision = 0;

// END SOLDIER VARS ////////////////////////////////////////////////////////////////////////////////////////////////

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
bool hasScout = false;

//
std::wstring s2ws(const std::string& s){
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

void debug (std::string s){
	std::wstring stemp = s2ws(s);
	OutputDebugString(stemp.c_str());
}

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

int distance (BWAPI::Position p1, BWAPI::Position p2){
	return (int) sqrt(pow((double)(p1.x() - p2.x()), 2) + pow((double)(p1.y() - p2.y()), 2));
}

bool isCommandCenter (Unidade* u){
	BWAPI::UnitType t = u->getType();

	return
		t == BWAPI::UnitTypes::Zerg_Infested_Command_Center ||
		t == BWAPI::UnitTypes::Protoss_Nexus ||
		t == BWAPI::UnitTypes::Terran_Command_Center
	;
}

bool isZealot (Unidade* u){
	return (u->getType() == BWAPI::UnitTypes::Protoss_Zealot && u->isCompleted());
}

bool isSoldier (Unidade* u){
	return (!(u->getType().isWorker()) && !(u->getType().isBuilding()));
}

bool seesEnemyStructure (Unidade* u){
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(e.empty()){
		return false;	
	}else{

		for (it = e.begin(); it != e.end(); ++it){
			Unidade* f = *it;
						
			if(u->isEnemy(f) && f->getType().isBuilding()){
				
				if(!foundFirstEnemyStructure){
					u->stop();
					firstEnemyStructureFoundPosition = f->getPosition();
					foundFirstEnemyStructure = true;
				}

				return true;
			}
		}

		return false;
	}
}

bool seesEnemyCommand (Unidade* u){
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(e.empty()){
		return false;	
	}else{

		for (it = e.begin(); it != e.end(); ++it){
			Unidade* f = *it;
			
			if(u->isEnemy(f) && isCommandCenter(f)){
				if(!foundEnemyCommand){
					enemyCommandPosition = f->getPosition();
					foundEnemyCommand = true;
				}

				return true;
			}

		}

		return false;
	}
}

bool seesMinerals (Unidade* u){
	std::set<Unidade*> e = u->getMinerals();
	std::set<Unidade*>::iterator it;
	bool found = false;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			Unidade* f = *it;
			mineralsFoundByScout.insert(f);
		}
	}

	return found;
}

bool seesEnemyWorker (Unidade* u){
	bool found = false;
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;
	Unidade* f;

	if(!e.empty()){
		for (it = e.begin(); it != e.end() && !found; ++it){
			f = *it;

			if(u->isEnemy(f) && f->getType().isWorker()){
				nextEnemyWorker = f;
				hasNextEnemyWorker = true;
				found = true;
			}
		}
	}

	return found;
}

void attackFirstEnemyWorker (Unidade* u){
	if(nextEnemyWorker){
		u->attack(nextEnemyWorker);
	}
}

char nextSector (Unidade* u){
	/*
		Retorna para qual setor o scout deve ir.
		A partir do setor atual, segue em um sentido horario.

		Considera-se que o scout atingiu o setor, quando ele estiver
		dentro de um raio de tolerancia do destino, definido por
		goalRadius
	*/
 
 	std::string dbgmsg;

	char currentSector = getSector((u->getPosition()));
	char next = currentSector;
	BWAPI::Position currentSectorCenter = getSectorCenter(currentSector);
	lastDistanceToNextSector = distance(u->getPosition(), currentSectorCenter);

	if(distance(u->getPosition(), currentSectorCenter) > goalRadius){
		next = currentSector;

		/*
			Verifica se a regiao de tolerancia (goalRadius) eh alcancavel. Tenta alcanca-la por 
			<maxAmountTryReachGoalRadius> se notar em algum momento que que a distancia nao diminui
			acrescenta uma tentativa.

			Se estourar as possibilidades aumenta o goalRadius. O goalRadius aumenta ateh atingir o
			batedor nesse caso ele passa para o proximo setor;
		*/
		if(distance(u->getPosition(), currentSectorCenter) >= lastDistanceToNextSector){
			nextSectorReachTryAmount = nextSectorReachTryAmount + 1;
		}

		if(nextSectorReachTryAmount >= maxAmountTryReachGoalRadius){
			goalRadius = goalRadius + goalRadiusDelta;
			nextSectorReachTryAmount = 0;
		}
	}else{
		if(currentSector == 'A') next = 'B';
		if(currentSector == 'B') next = 'C';
		if(currentSector == 'C') next = 'M';
		if(currentSector == 'K') next = 'L';
		if(currentSector == 'L') next = 'A';
		if(currentSector == 'M') next = 'Z';
		if(currentSector == 'X') next = 'K';
		if(currentSector == 'Y') next = 'X';
		if(currentSector == 'Z') next = 'Y';

		// reseta os parametros
		goalRadius = 50;
		nextSectorReachTryAmount = 0;
	}

	return next;
}

BWAPI::Position nextSpiralPosition (Unidade* u, BWAPI::Position center){
	/*
		Faz o batedor andar em expiral, em torno da primeira unidade encontrada.

		Para cada volta, divide a circunferencia em quatro setores que serao para onde
		o batedor ira se mover. Se no final de uma volta, nao encontrar o centro de comando
		inicia mais uma volta porem dessa vez, com o raio maior em spiralRadiusDelta
	*/

	int spiralRadius = (spiralTurn*spiralRadiusDelta);

	if(distance(u->getPosition(), nextSpiralSectorPosition) > spiralGoalRadius){
		if(distance(u->getPosition(), nextSpiralSectorPosition) >= lastDistanceToNextSpiralSector){
			nextSpiralSectorReachTryAmount = nextSpiralSectorReachTryAmount + 1;
		}

		if(nextSpiralSectorReachTryAmount >= maxAmountTryReachSpiralGoalRadius){
			spiralSector = spiralSector + 1;
			nextSpiralSectorReachTryAmount = 0;
		}
	}else{
		nextSpiralSectorReachTryAmount = 0;
		spiralSector = spiralSector + 1;
	}

	if(spiralSector == 0){
		nextSpiralSectorPosition = BWAPI::Position(center.x(), (center.y() - spiralRadius));
	}else if(spiralSector == 1){
		nextSpiralSectorPosition = BWAPI::Position((center.x() + spiralRadius), center.y());
	}else if(spiralSector == 2){
		nextSpiralSectorPosition = BWAPI::Position(center.x(), (center.y() + spiralRadius));
	}else if(spiralSector == 3){
		nextSpiralSectorPosition = BWAPI::Position((center.x() - spiralRadius), center.y());

		// fim da volta. Incrementa a volta e reinicia o sector
		spiralTurn = spiralTurn + 1;
		spiralSector = -1;
	}

	return nextSpiralSectorPosition;
}

void scoutVision (Unidade* u){
	double now = std::clock() / (double) CLOCKS_PER_SEC;

	if(now > nextVisionCheck){
		if(seesEnemyStructure(u)){
			seesEnemyCommand(u);
		}

		seesMinerals(u);

		zealotCountVision = zealotCountVision + 1;

		if(zealotCountVision >= zealotCount){
			zealotCountVision = 0;
			nextVisionCheck = now + visionCheckInterval;
		}
	}
}

void AIBatedor (Unidade* u){

    /*
		A primeira etapa do para o scout seria dividir o mapa em quadrantes.

		Geralmente, os combates em SC, os jogadores inimigos se localizam em quadrantes opostos
    */

	if(foundFirstEnemyStructure){

		if(foundEnemyCommand){
			if(seesEnemyWorker(u)){
				attackFirstEnemyWorker(u);
			}else{
				u->move(nextSpiralPosition(u, enemyCommandPosition));
			}
		}else{
			u->move(nextSpiralPosition(u, firstEnemyStructureFoundPosition));
		}

	}else{
		u->rightClick(getSectorCenter(nextSector(u)));
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

		if(u == scout) {
			scoutVision(u);
		};

		//Inserir o codigo de voces a partir daqui//
		if(u->isIdle()){ //nao ta fazendo nada, fazer algo util
			if(u == scout) AIBatedor(u);
			else if(u->getType().isWorker() && u!=Selected_Worker){AITrabalhador(u);}
			else {caboSoldado[0] = AIGuerreiro(caboSoldado);}
		}
		//
		Sleep(10);//Sempre dormir pelo menos 10ms no final do loop, pois uma iteracao da thread é muito mais rápida do que um turno do bwapi.
	}
}



bool seekEnemyWorker(Unidade* u){
	bool found = false;

	if(foundEnemyCommand){

		if(u->getDistance(enemyCommandPosition) < 50){
			std::set<Unidade*> e = u->getEnemyUnits();
			std::set<Unidade*>::iterator it;
			bool attackingWorker = false;

			if(!e.empty()){
				
				for (it = e.begin(); it != e.end() && !attackingWorker; ++it){
					Unidade* f = *it;
					
					if(u->isEnemy(f) && f->getType().isWorker()){
						u->attack(f);
						attackingWorker = true;
						found = true;
					}
				}
			}
		}else{
			u->move(enemyCommandPosition);
		}
	}

	return found;
}

bool seekEnemyCommandCenter (Unidade* u){
	bool found = false;

	if(foundEnemyCommand){
		found = true;
		u->attack(enemyCommandPosition);
	}

	return found;
}

void soldierVision (Unidade* u){
	double now = std::clock() / (double) CLOCKS_PER_SEC;

	if(now > nextSoldierVisionCheck){

		if(!seekEnemyWorker(u)){
			if(!seekEnemyCommandCenter(u)){
				debug("kill structs...\n");
			}else{
				debug("kill command center...\n");
			}
		}else{
			debug("kill workers...\n");
		}

		nextSoldierVisionCheck = now + soldierVisionCheckInterval;
	}
}

void updateSoldiers(){
	std::set<Unidade*> unidades = Protoss_Nexus->getAllyUnits();
	Unidade* f;
	zealotCount = 0;

	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++) {
		f = *it;
		if(isZealot(f)){
			zealotCount = zealotCount + 1;
		}
	}

	hasEnoughSoldiers = ((zealotCount / 4) >= 1);

	if(hasEnoughSoldiers){
		for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++) {
			f = *it;
			if(isZealot(f)){
				soldierVision(f);
			}
		}
	}
}

DWORD WINAPI general_militar(LPVOID param){

	Unidade* u = (Unidade*) param;

	while(true){
		//Se houve algum problema (ex: o jogo foi fechado) ou a unidade estah morta, finalizar a thread
		if(GameOver) return 0;
		
		if(Protoss_Nexus != NULL && Protoss_Nexus->exists() && !Protoss_Nexus->checkNovoTurno()){
			Sleep(10);
			continue;
		}
		//Inserir o codigo de voces a partir daqui//
		// Codigo General Militar

		updateSoldiers();
		
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
	{
		if ((*it)->getType().isWorker())
		{
			Protoss_Workers [cont] = (*it);
			cont++;
		}
	}
	numWorkers = cont;
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
		base = Protoss_Nexus->getPosition();
		int halfWidth = (AgentePrincipal::mapWidth()*16);
		int halfHeight = (AgentePrincipal::mapHeight()*16);
		centro = BWAPI::Position(halfWidth, halfHeight);
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


