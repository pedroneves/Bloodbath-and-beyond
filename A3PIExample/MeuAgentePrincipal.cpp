#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
#include <stdio.h>
#include <math.h>
#include <ctime>
#include <set>

using namespace BWAPI;
 
//blackboard!
BWAPI::Position centro;
BWAPI::Position base;
BWAPI::Position base_inimiga;



Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

// Scout vars ////////////////////////////////////////////////////////////////////////////////////////////////////
Unidade* scout;

// Informa qual o setor inicial
char startingSector;
std::string searchSequence;
// Informa para qual setor o scout esta indo
char currentDestSector; 
// Raio de tolerancia para o centro do setor
int goalRadius = 15;
// Em quanto o raio de tolerancia vai aumentar
int goalRadiusDelta = 85;
// Verificador para informar a ultima distancia conhecida para o centro do setor
double lastDistanceToNextSector = 100000;
// Quantidade de vezes que se tentou chegar ao centro do setor
int nextSectorReachTryAmount = 0;
// Quantidade maxima de vezes que se pode tentar chegar ao setor
int maxAmountTryReachGoalRadius = 2;
// Informa quantos setores foram visitados
int checkedSectorsCount = 0;


// Posicao da primeira estrutura inimiga encontrada
BWAPI::Position firstEnemyStructureFoundPosition;
// Flag se encontrou alguma estrutura inigmiga
bool foundFirstEnemyStructure = false;
// Posicao do centro de comando inimigo
BWAPI::Position enemyCommandPosition;
// Flag se encontrou centro de comando inigmigo
bool foundEnemyCommand = false;

char localSearchSector = 'A';
bool isSearchingInsideSector = false;
int corner = 0;
int cornerGoalRadius = 25;
int cornerGoalRadiusDelta = 120;
int cornerReachTryCount = 0;
int maxCornerReachTries = 1;
double lastDistanceToNextCorner = 10000;


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
double scoutVisionCheckInterval = 1;
// Controle para saber se deve checar ou nao
double nextScoutVisionCheck = (std::clock() / ((double) CLOCKS_PER_SEC)) + scoutVisionCheckInterval;


// Primeiro worker inimigo encontrado
Unidade* nextEnemyWorker;
// Flag para saber se encontrou algum inimigo
bool hasNextEnemyWorker = false;


// Minerais encontrados enquanto procurava o inimigo
std::set<Unidade*> mineralsFoundByScout;

// END SCOUT VARS //////////////////////////////////////////////////////////////////////////////////////////////////


// Soldier vars ////////////////////////////////////////////////////////////////////////////////////////////////////

int enemyCommandGoalRadius = 50;
int zealotCountAtBase = 0;
bool hasEnoughSoldiersToStart = false;
bool hasEnoughSoldiersToAttack = false;

// Quantidade em segundos que se vai checar a visao
double soldierVisionCheckInterval = 1;
// Controle para saber se deve checar ou nao
double nextSoldierVisionCheck = (std::clock() / ((double) CLOCKS_PER_SEC)) + soldierVisionCheckInterval;
int zealotCountVision = 0;

int minimumAmountOfSoldiersToStartAttack = 5;
int minimumAmountOfSoldiersToAttack = 3;
int protectionCCRadius = 45;
// END SOLDIER VARS ////////////////////////////////////////////////////////////////////////////////////////////////

//RECURSOS//
Unidade* Protoss_Gateways [10];
Unidade* Protoss_Pylons [10];
Unidade* Protoss_Workers [10];
std::set<Unidade*> All_Unities;
Unidade* Selected_Worker;
int numWorkers;
int numGateways;
int numPylons;
bool resourceSemaphore;
bool pylonBuildingSemaphore;



bool GameOver = false;
bool hasScout = false;

double now () {
	return (std::clock() / ((double) CLOCKS_PER_SEC));
}

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

bool isInSameSector (BWAPI::Position p1, BWAPI::Position p2){
	return (getSector(p1) == getSector(p2));
}

std::string getSectorSearchSequence (char initialSector){
	if(initialSector == 'A') { return "AZMYXKCBL"; }
	if(initialSector == 'B') { return "BYXZMCKAL"; }
	if(initialSector == 'C') { return "CXYKABZML"; }
	if(initialSector == 'M') { return "MKAXYZBCL"; }
	if(initialSector == 'Z') { return "ZABKXYCML"; }
	if(initialSector == 'Y') { return "YBACMZKXL"; }
	if(initialSector == 'X') { return "XCBMZYAKL"; }
	if(initialSector == 'K') { return "KMCZYXBAL"; }
	if(initialSector == 'L') { return "LBYZMCAKX"; }
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
		t == BWAPI::UnitTypes::Zerg_Hatchery || 
		t == BWAPI::UnitTypes::Zerg_Lair || 
		t == BWAPI::UnitTypes::Zerg_Hive || 
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

Unidade* seesEnemyUnit (Unidade* u){
	Unidade* retorno = NULL;
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			if(u->isEnemy((*it)) && (!(*it)->getType().isBuilding())){
				retorno = *it;
			}
		}
	}

	return retorno;
}

Unidade* seesEnemyStructure (Unidade* u){
	Unidade* retorno = NULL;
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			if(u->isEnemy((*it)) && (*it)->getType().isBuilding()){
				retorno = *it;
			}
		}
	}

	return retorno;
}

Unidade* seesEnemyCommand (Unidade* u){
	Unidade* retorno = NULL;
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			if(u->isEnemy((*it)) && isCommandCenter((*it))){
				retorno = *it;
			}
		}
	}

	return retorno;
}

bool seesMinerals (Unidade* u){
	std::set<Unidade*> e = u->getMinerals();
	std::set<Unidade*>::iterator it;
	bool found = false;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			Unidade* f = *it;

			if(enemyCommandPosition != NULL){
				if(!isInSameSector(u->getPosition(), enemyCommandPosition)){
					mineralsFoundByScout.insert(f);
					debug("Minerals found! Total: " + SSTR(mineralsFoundByScout.size()) + "\n");
				}
			}else{
				mineralsFoundByScout.insert(f);
				debug("Minerals found! Total: " + SSTR(mineralsFoundByScout.size()) + "\n");
			}
		}
		found = true;
	}

	return found;
}

Unidade* seesEnemyWorker (Unidade* u){
	Unidade* retorno = NULL;
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(!e.empty()){
		for (it = e.begin(); it != e.end(); ++it){
			if(u->isEnemy((*it)) && (*it)->getType().isWorker()){
				retorno = *it;
			}
		}
	}

	return retorno;
}

void attackFirstEnemyWorker (Unidade* u){
	if(nextEnemyWorker){
		u->attack(nextEnemyWorker);
	}
}

BWAPI::Position getSectorCornerPosition (char s, int c){
	int width = centro.x()*2;
	int height = centro.y()*2;

	double C1 = width / 3;
	double C2 = (2*width) / 3;

	double L1 = height / 3;
	double L2 = (2*height) / 3;

	double halfSectorWidth = (C1 / 2);
	double halfSectorHeight = (L1 / 2);

	BWAPI::Position sectorCenter = getSectorCenter(s);

	if(c == 0) return BWAPI::Position((int) (sectorCenter.x() - halfSectorWidth), (int) (sectorCenter.y() - halfSectorHeight));
	if(c == 1) return BWAPI::Position((int) (sectorCenter.x() + halfSectorWidth), (int) (sectorCenter.y() - halfSectorHeight));
	if(c == 2) return BWAPI::Position((int) (sectorCenter.x() - halfSectorWidth), (int) (sectorCenter.y() + halfSectorHeight));
	if(c == 3) return BWAPI::Position((int) (sectorCenter.x() + halfSectorWidth), (int) (sectorCenter.y() + halfSectorHeight));
}

void moveInsideSector (Unidade* u){

	BWAPI::Position cornerPos = getSectorCornerPosition(localSearchSector, corner);
	lastDistanceToNextCorner = distance(u->getPosition(), cornerPos);
	
	if(distance(u->getPosition(), cornerPos) > cornerGoalRadius){

		if(distance(u->getPosition(), cornerPos) >= lastDistanceToNextCorner){
			cornerReachTryCount++;
		}

		if(cornerReachTryCount >= maxCornerReachTries){
			cornerGoalRadius = cornerGoalRadius + cornerGoalRadiusDelta;
			cornerReachTryCount = 0;
		}

		u->rightClick(cornerPos);
	}else{

		// reseta os parametros
		lastDistanceToNextCorner = 10000;
		cornerGoalRadius = 25;
		cornerReachTryCount = 0;
		
		corner++;

		if(corner > 3){
			corner = 0;
			isSearchingInsideSector = false;
		}else{
			u->rightClick(getSectorCornerPosition(localSearchSector, corner));
		}
	}
}

void moveNextSector (Unidade* u){
	/*
		Retorna para qual setor o scout deve ir.
		A partir do setor atual, segue em um sentido horario.

		Considera-se que o scout atingiu o setor, quando ele estiver
		dentro de um raio de tolerancia do destino, definido por
		goalRadius
	*/

	currentDestSector = getSector((u->getPosition()));
	BWAPI::Position currentSectorCenter = getSectorCenter(currentDestSector);
	lastDistanceToNextSector = distance(u->getPosition(), currentSectorCenter);
 
	if(distance(u->getPosition(), currentSectorCenter) > goalRadius){
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

		if(isSearchingInsideSector){
			moveInsideSector(u);
		}else{
			checkedSectorsCount = (int) (checkedSectorsCount + 1) % 9;

			// reseta os parametros
			goalRadius = 50;
			nextSectorReachTryAmount = 0;
			isSearchingInsideSector = true;

			currentDestSector = searchSequence[checkedSectorsCount];
			localSearchSector = currentDestSector;
			
			u->rightClick(getSectorCenter(currentDestSector));
		}
	}
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

	Unidade* enemyCommand;
	Unidade* enemyStructure;

	if(now() > nextScoutVisionCheck){
		
		if(seesEnemyStructure(u)){
			if(!foundFirstEnemyStructure){
				debug("Enemy STRUCTURE found!\n");
				foundFirstEnemyStructure = true;
				enemyStructure = seesEnemyStructure(u);
				firstEnemyStructureFoundPosition = enemyStructure->getPosition();
			}

			if(seesEnemyCommand(u)){
					if(!foundEnemyCommand){
					debug("Enemy COMMAND found!\n");
					foundEnemyCommand = true;
					enemyCommand = seesEnemyCommand(u);
					enemyCommandPosition = enemyCommand->getPosition();
				}
			}
		}

		seesMinerals(u);

		nextScoutVisionCheck = now() + scoutVisionCheckInterval;
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
				u->attack(seesEnemyWorker(u));
			}else{
				u->move(nextSpiralPosition(u, enemyCommandPosition));
			}
		}else{
			u->move(nextSpiralPosition(u, firstEnemyStructureFoundPosition));
		}
	}else{
		moveNextSector(u);
	}
}

void AITrabalhador (Unidade* u){
	double dist = 99999;
	Unidade* mineralPerto = NULL;
	Unidade* f = NULL;
	
	if(seesEnemyWorker(u))
	{
		u->attack(seesEnemyWorker(u));
	}else
	{
		if(Protoss_Nexus != NULL)
		{
			std::set<Unidade*> minerais = Protoss_Nexus->getMinerals();
	
			if(!minerais.empty())
			{
				for(std::set<Unidade*>::iterator it = minerais.begin(); it != minerais.end(); it++)
				{
					if(Protoss_Nexus->getDistance(*it) < dist)
					{
						dist = Protoss_Nexus->getDistance(*it);
						mineralPerto = *it;
					}
				}
		
			}else 
			{
				for(std::set<Unidade*>::iterator it = mineralsFoundByScout.begin(); it != mineralsFoundByScout.end(); it++)
				{
					f = *it;

					if(f != NULL && f->exists() && distance(Protoss_Nexus->getPosition(), f->getPosition()) < dist)
					{
						dist = distance(Protoss_Nexus->getPosition(), f->getPosition());
						mineralPerto = f;
					}
				}
			}
	
			if(mineralPerto != NULL){
				u->rightClick(mineralPerto);
			}
			if(mineralPerto == NULL){
				u->move(Protoss_Nexus->getPosition());
			}
		}
	}
}
//Modificado

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
			if(!startingSector){
				debug("Accquiring initial sector...");
				startingSector = getSector(u->getPosition());
				debug("Done: " + SSTR(startingSector) + "\nAccquiring sector search sequence...");
				searchSequence = getSectorSearchSequence(startingSector);
				debug("Done: " + searchSequence + "\n");
			}
			scoutVision(u);
		}

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
	if(u){
		if(foundEnemyCommand){
			if(u->getDistance(enemyCommandPosition) < 50){
				if(seesEnemyWorker(u)){
					u->attack(seesEnemyWorker(u));
					return true;
				}else{
					return false;
				}
			}else{
				u->move(enemyCommandPosition);
				return false;
			}
		}else{
			return false;
		}
	}else{
		return false;
	}
}

bool seekEnemyCommandCenter (Unidade* u){
	Unidade* enemyCc = seesEnemyCommand(u);

	if(u){
		if(foundEnemyCommand){
			if(enemyCc){
				u->attack(enemyCc);
				return true;
			}else{
				u->move(enemyCommandPosition);
				return false;
			}
		}else{
			return false;
		}
	}else{
		return false;
	}
}

bool seekAnyEnemy (Unidade* u){
	Unidade* enemy = seesEnemyUnit(u);

	if(u){
		if(enemy == NULL){
			enemy = seesEnemyStructure(u);
		}

		if(enemy != NULL){
			u->attack(enemy);
			return true;
		}else{
			return false;
		}
	}else{
		return false;
	}
}

bool mayAttack (){
	return (hasEnoughSoldiersToStart && hasEnoughSoldiersToAttack);
}

void soldierVision (Unidade* u){

	if(u){
		if(now() > nextSoldierVisionCheck){

			if(mayAttack()){
				if(!seekEnemyWorker(u)){
					if(!seekEnemyCommandCenter(u)){
						seekAnyEnemy(u);
					}
				}
			}else{
				seekAnyEnemy(u);
			}

			zealotCountVision = zealotCountVision + 1;
			
			if(zealotCountVision >= zealotCountAtBase){
				zealotCountVision = 0;
				nextSoldierVisionCheck = now() + soldierVisionCheckInterval;
			}
		}
	}
}

void updateSoldiers(){
	std::set<Unidade*> unidades = Protoss_Nexus->getAllyUnits();
	Unidade* f;
	zealotCountAtBase = 0;

	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++) {
		f = *it;
		if(isZealot(f) && f->isIdle()){
			if(isInSameSector(f->getPosition(), Protoss_Nexus->getPosition())){
				zealotCountAtBase = zealotCountAtBase + 1;
			}
		}
	}

	if(hasEnoughSoldiersToStart){
		hasEnoughSoldiersToAttack = (zealotCountAtBase >= minimumAmountOfSoldiersToAttack);
	}

	if(!hasEnoughSoldiersToStart){
		hasEnoughSoldiersToStart = (zealotCountAtBase >= minimumAmountOfSoldiersToStartAttack);
	}

	for(std::set<Unidade*>::iterator it = unidades.begin(); it != unidades.end(); it++) {
		f = *it;
		if(isZealot(f)){
			soldierVision(f);

			int randX = (rand() % protectionCCRadius);
			int randY = (rand() % protectionCCRadius);
			if(rand()%2 == 1){randX = 0 - randX;}
			if(rand()%2 == 1){randY = 0 - randY;}
			if(
				f->isIdle() && 
				!mayAttack() && 
				isInSameSector(f->getPosition(), Protoss_Nexus->getPosition()) && 
				f->getDistance(Protoss_Nexus) > protectionCCRadius
			){
				f->move(BWAPI::Position(Protoss_Nexus->getPosition().x() + randX, Protoss_Nexus->getPosition().y() + randY ));
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

void updateBlackBoard(){
	int w = 0;
	int p = 0;
	int g = 0;
	for(std::set<Unidade*>::iterator it = All_Unities.begin(); it != All_Unities.end(); it++)
	{
		if ((*it)->getType().isWorker() && (*it)!=scout)
		{
			Protoss_Workers [w] = (*it);
			w++;
		}
		else if ((*it)->getType().isWorker() && (*it)==scout)
		{
			seesMinerals(*it);
		}
		else if ((*it)->getType() == BWAPI::UnitTypes::Protoss_Pylon)
		{
			Protoss_Pylons [p] = (*it);
			p++;
			
			seesMinerals(*it);
			
		}else if ((*it)->getType() == BWAPI::UnitTypes::Protoss_Gateway)
		{
			Protoss_Gateways [g] = (*it);
			g++;
			
			seesMinerals(*it);
		}


	}
	numWorkers = w;
	numPylons = p;
	numGateways = g;

}

void buildPylon(){

	resourceSemaphore = true;
	
Unidade* nexus = Protoss_Nexus;
Unidade* worker = Protoss_Workers[numWorkers-1];

//nexus->getDistance();
//nexus->getPosition();

BWAPI::TilePosition aroundTile = nexus->getTilePosition();

int minDist = 8;
int stopDist = 40;

//debug("Pylon \n");
       

 while(!worker->isConstructing()) {
	for (int i=aroundTile.x(); (i<=aroundTile.x() + minDist) && !worker->isConstructing(); i++) {
		for (int j=aroundTile.y(); (j<=aroundTile.y() + minDist) && !worker->isConstructing(); j++) {

			//debug("1 Position: " + SSTR(i) + " " + SSTR(j) + "\n");

				if (worker->isBuildable(BWAPI::TilePosition(i,j))) {
					//debug("2 Position: " + SSTR(i) + " " + SSTR(j) + "\n");
							worker->build(BWAPI::TilePosition(i,j),UnitTypes::Protoss_Pylon);

				}
			}
		}

minDist+2;
}




resourceSemaphore = false;

//  bool x = u->hasPower(3,4,50,60);
//	u->isBuildable(50,50);
//	u->isBuildable(BWAPI::TilePosition(3,5));



	
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
		 updateBlackBoard();
		//updateWorkers();
		//updatePylons();
		//updateGateways();

		if(Protoss_Nexus->minerals() > 50 && numWorkers < 6 && !Protoss_Nexus->isTraining()){
			Protoss_Nexus->train(BWAPI::UnitTypes::Protoss_Probe);		
		}

		if(Protoss_Nexus->minerals() > 100 && ((numWorkers == 6 && numPylons==0) || (avaiableSupply()<3)) && resourceSemaphore == false){
			buildPylon();
		}

		if(Protoss_Nexus->minerals() > 150 && numPylons>0 && resourceSemaphore == false && numGateways < 2){
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
		CreateThread(NULL,0,general_recursos,NULL,0,NULL);
		CreateThread(NULL,0,general_militar,NULL,0,NULL);

	}
	else if(tipo == BWAPI::UnitTypes::Protoss_Gateway){
		Protoss_Gateways[numGateways] = unidade;
		numGateways = numGateways+1;
		All_Unities.insert(unidade);
	}else if(tipo == BWAPI::UnitTypes::Protoss_Pylon){
		Protoss_Pylons[numPylons] = unidade;
		numPylons = numPylons+1;
		All_Unities.insert(unidade);
	}
	//Nao desperdicar threads com predios que nao fazem nada
	else if(!tipo.canProduce()){
		if(tipo == BWAPI::UnitTypes::Protoss_Probe){
			if(!hasScout)
			{
				hasScout = true;
				scout = unidade;
			}else{
				All_Unities.insert(unidade);
			}
		}
		CreateThread(NULL,0,threadAgente,(void*)unidade,0,NULL);
	}
}

/*
	Os outros eventos nao existem numa arquitetura orientada a Agentes Autonomos, pois eram relacionados ao Player do Broodwar
	de maneira generica, nao sendo especificamente ligados a alguma unidade do jogador. Se desejado, seus comportamentos podem
	ser simulados através de técnicas ou estruturas de comunicação dos agentes, como por exemplo Blackboards.
*/


