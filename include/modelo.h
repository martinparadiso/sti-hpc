/* Demo_03_Model.h */

#ifndef DEMO_03_MODEL
#define DEMO_03_MODEL

#include <boost/mpi.hpp>
#include <iostream>
#include <fstream>
#include "repast_hpc/Schedule.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/SharedContext.h"
#include "repast_hpc/AgentRequest.h"
#include "repast_hpc/TDataSource.h"
#include "repast_hpc/SVDataSet.h"
#include "repast_hpc/SharedDiscreteSpace.h"
#include "repast_hpc/GridComponents.h"
#include "repast_hpc/AgentRequest.h"

#include "agente.h"
#include "plano.h"


//Clases para enviar y recibir paquetes de agentes

//Paquete para enviar agentes
class RepastHPCAgentePackageProvider {
	
private:
    repast::SharedContext<Agente>* agents;
	
public:
    RepastHPCAgentePackageProvider(repast::SharedContext<Agente>* agentPtr);
    void providePackage(Agente * agent, std::vector<RepastHPCAgentePackage>& out);
    void provideContent(repast::AgentRequest req, std::vector<RepastHPCAgentePackage>& out);

};


//Paquete para recibir agentes
class RepastHPCAgentePackageReceiver {
	
private:
    repast::SharedContext<Agente>* agents;
	
public:
    RepastHPCAgentePackageReceiver(repast::SharedContext<Agente>* agentPtr);
    Agente * createAgent(RepastHPCAgentePackage package);
    void updateAgent(RepastHPCAgentePackage package);
	
};



class Modelo {
	int stopAt;
	int _cant_agentes_act;
	const int _rank;
	repast::Properties* props;
	repast::SharedContext<Agente> context;

	RepastHPCAgentePackageProvider* provider;
	RepastHPCAgentePackageReceiver* receiver;

    std::ifstream _mapa_archivo;
	Plano * _plano;
    repast::SharedDiscreteSpace<Agente, repast::StrictBorders, repast::SimpleAdder<Agente> >* discreteSpace;

	std::ofstream _arch_salida;
	
public:
	Modelo(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm);
	~Modelo();
	void init();
	void doSomething();
	void initSchedule(repast::ScheduleRunner& runner);
	void recordResults();
};

#endif
