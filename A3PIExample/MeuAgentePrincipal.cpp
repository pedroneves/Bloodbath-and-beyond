#include "MeuAgentePrincipal.h"
#include <AgentePrincipal.h>
#include <Windows.h>
#include <BWAPI\Position.h>
#include <BWAPI\UnitType.h>
#include <stdio.h>
#include <math.h>
#include <ctime>
 
//blackboard!
BWAPI::Position centro;
BWAPI::Position base;
BWAPI::Position base_inimiga;

Unidade* Protoss_Nexus;
Unidade* Protoss_Gateway;

// Scout vars
Unidade* scout;
char currentDestSector;
bool isInLoop = false;
int goalRadius = 50;
double lastDistanceToNextSector = 100000;
int nextSectorReachTryAmount = 0;
int maxAmountTryReachGoalRadius = 3;

BWAPI::Position firstEnemyStructureFoundPosition;
bool foundFirstEnemyStructure = false;
BWAPI::Position enemyCommandPosition;
bool foundEnemyCommand = false;

int spiralTurn = 1;
int spiralSector = 0;
int spiralRadiusDelta = 35;
int spiralGoalRadius = 5;
BWAPI::Position nextSpiralSectorPosition;
double lastDistanceToNextSpiralSector;
int nextSpiralSectorReachTryAmount = 0;
int maxAmountTryReachSpiralGoalRadius = 3;

double enemyCheckInterval = 1;
double nextEnemyCheck = (std::clock() / ((double) CLOCKS_PER_SEC)) + enemyCheckInterval;

// END SCOUT VARS

bool GameOver = false;
bool hasScout = false;
Unidade* amigoDaVez = NULL;
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

int distance (BWAPI::Position p1, BWAPI::Position p2){
	return (int) sqrt(pow((double)(p1.x() - p2.x()), 2) + pow((double)(p1.y() - p2.y()), 2));
}

bool isStructure (Unidade* u){
	BWAPI::UnitType t = u->getType();

	return
		t == BWAPI::UnitTypes::Zerg_Infested_Command_Center ||
		t == BWAPI::UnitTypes::Zerg_Hatchery ||
		t == BWAPI::UnitTypes::Zerg_Lair ||
		t == BWAPI::UnitTypes::Zerg_Hive ||
		t == BWAPI::UnitTypes::Zerg_Nydus_Canal ||
		t == BWAPI::UnitTypes::Zerg_Hydralisk_Den ||
		t == BWAPI::UnitTypes::Zerg_Defiler_Mound ||
		t == BWAPI::UnitTypes::Zerg_Greater_Spire ||
		t == BWAPI::UnitTypes::Zerg_Queens_Nest ||
		t == BWAPI::UnitTypes::Zerg_Evolution_Chamber ||
		t == BWAPI::UnitTypes::Zerg_Ultralisk_Cavern ||
		t == BWAPI::UnitTypes::Zerg_Spire ||
		t == BWAPI::UnitTypes::Zerg_Spawning_Pool ||
		t == BWAPI::UnitTypes::Zerg_Creep_Colony ||
		t == BWAPI::UnitTypes::Zerg_Spore_Colony ||
		t == BWAPI::UnitTypes::Zerg_Sunken_Colony ||
		t == BWAPI::UnitTypes::Zerg_Extractor ||
		t == BWAPI::UnitTypes::Protoss_Nexus ||
		t == BWAPI::UnitTypes::Protoss_Robotics_Facility ||
		t == BWAPI::UnitTypes::Protoss_Pylon ||
		t == BWAPI::UnitTypes::Protoss_Assimilator ||
		t == BWAPI::UnitTypes::Protoss_Observatory ||
		t == BWAPI::UnitTypes::Protoss_Gateway ||
		t == BWAPI::UnitTypes::Protoss_Photon_Cannon ||
		t == BWAPI::UnitTypes::Protoss_Citadel_of_Adun ||
		t == BWAPI::UnitTypes::Protoss_Cybernetics_Core ||
		t == BWAPI::UnitTypes::Protoss_Templar_Archives ||
		t == BWAPI::UnitTypes::Protoss_Forge ||
		t == BWAPI::UnitTypes::Protoss_Stargate ||
		t == BWAPI::UnitTypes::Protoss_Fleet_Beacon ||
		t == BWAPI::UnitTypes::Protoss_Arbiter_Tribunal ||
		t == BWAPI::UnitTypes::Protoss_Robotics_Support_Bay ||
		t == BWAPI::UnitTypes::Protoss_Shield_Battery ||
		t == BWAPI::UnitTypes::Terran_Command_Center ||
		t == BWAPI::UnitTypes::Terran_Comsat_Station ||
		t == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
		t == BWAPI::UnitTypes::Terran_Supply_Depot ||
		t == BWAPI::UnitTypes::Terran_Refinery ||
		t == BWAPI::UnitTypes::Terran_Barracks ||
		t == BWAPI::UnitTypes::Terran_Academy ||
		t == BWAPI::UnitTypes::Terran_Factory ||
		t == BWAPI::UnitTypes::Terran_Starport ||
		t == BWAPI::UnitTypes::Terran_Control_Tower ||
		t == BWAPI::UnitTypes::Terran_Science_Facility ||
		t == BWAPI::UnitTypes::Terran_Covert_Ops ||
		t == BWAPI::UnitTypes::Terran_Physics_Lab ||
		t == BWAPI::UnitTypes::Terran_Machine_Shop ||
		t == BWAPI::UnitTypes::Terran_Engineering_Bay ||
		t == BWAPI::UnitTypes::Terran_Armory ||
		t == BWAPI::UnitTypes::Terran_Missile_Turret ||
		t == BWAPI::UnitTypes::Terran_Bunker ||
		u->isBeingConstructed();
	;
}

bool isCommandCenter (Unidade* u){
	BWAPI::UnitType t = u->getType();

	return
		t == BWAPI::UnitTypes::Zerg_Infested_Command_Center ||
		t == BWAPI::UnitTypes::Protoss_Nexus ||
		t == BWAPI::UnitTypes::Terran_Command_Center
	;
}

bool seesEnemyStructure (Unidade* u){
	std::set<Unidade*> e = u->getEnemyUnits();
	std::set<Unidade*>::iterator it;

	if(e.empty()){
		debug("I see no enemy units\n");
		return false;	
	}else{

		for (it = e.begin(); it != e.end(); ++it){
			Unidade* f = *it;
						
			if(u->isEnemy(f) && isStructure(f)){
				
				debug("I see enemy structures!\n");				

				if(!foundFirstEnemyStructure){
					u->stop();
					firstEnemyStructureFoundPosition = f->getPosition();
					foundFirstEnemyStructure = true;
					debug("First Enemy structure found at " + SSTR(firstEnemyStructureFoundPosition.x()) + " " + SSTR(firstEnemyStructureFoundPosition.y()) + "\n");
				}

				return true;
			}
		}

		debug("\nNah...No structures, just units =/\n");

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
					debug("Enemy command found at " + SSTR(enemyCommandPosition.x()) + " " + SSTR(enemyCommandPosition.y()) + "\n");
				}

				return true;
			}

		}

		return false;
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
			goalRadius = goalRadius + 50;
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

 	dbgmsg = "Next sector ";
	dbgmsg = dbgmsg + " is '";
	dbgmsg = dbgmsg + next;
	dbgmsg = dbgmsg + "' with goalRadius: ";
	dbgmsg = dbgmsg + SSTR(goalRadius);
	dbgmsg = dbgmsg + " | Try: ";
	dbgmsg = dbgmsg + SSTR(nextSectorReachTryAmount);
	dbgmsg = dbgmsg + " | Max: ";
	dbgmsg = dbgmsg + SSTR(maxAmountTryReachGoalRadius); 

	debug(dbgmsg + "\n");

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

	debug("Walking in spiral. Turn: " + SSTR(spiralTurn) + " | Sector: " + SSTR(spiralSector) + " | Tries: " + SSTR(nextSpiralSectorReachTryAmount) + "\n");

	return nextSpiralSectorPosition;
}

void scoutVision (Unidade* u){
	double now = std::clock() / (double) CLOCKS_PER_SEC;

	if(now > nextEnemyCheck){
		if(seesEnemyStructure(u)){
			seesEnemyCommand(u);
		}

		nextEnemyCheck = now + enemyCheckInterval;
	}
}

void AIBatedor (Unidade* u){

    /*
		A primeira etapa do para o scout seria dividir o mapa em quadrantes.

		Geralmente, os combates em SC, os jogadores inimigos se localizam em quadrantes opostos
    */

	if(foundFirstEnemyStructure){

		if(foundEnemyCommand){
			u->move(nextSpiralPosition(u, enemyCommandPosition));
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


