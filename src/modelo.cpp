/* Demo_03_Model.cpp */

#include <vector>
#include <regex>
#include <fstream>
#include <iostream>
#include <boost/mpi.hpp>
#include "repast_hpc/AgentId.h"
#include "repast_hpc/RepastProcess.h"
#include "repast_hpc/Utilities.h"
#include "repast_hpc/Properties.h"
#include "repast_hpc/initialize_random.h"
#include "repast_hpc/SVDataSetBuilder.h"
#include "repast_hpc/Point.h"

#include "modelo.h"
#include "plano.h"

RepastHPCAgentePackageProvider::RepastHPCAgentePackageProvider(repast::SharedContext<Agente>* agentPtr): agents(agentPtr){ }

void RepastHPCAgentePackageProvider::providePackage(Agente * agent, std::vector<RepastHPCAgentePackage>& out){
    repast::AgentId id = agent->getId();
    RepastHPCAgentePackage package(id.id(), id.startingRank(), id.agentType(), id.currentRank(), agent->get_prob_contagiar(), agent->get_prob_ser_contagiado(), agent->get_tipo());
    out.push_back(package);
}

void RepastHPCAgentePackageProvider::provideContent(repast::AgentRequest req, std::vector<RepastHPCAgentePackage>& out){
    std::vector<repast::AgentId> ids = req.requestedAgents();
    for(size_t i = 0; i < ids.size(); i++){
        providePackage(agents->getAgent(ids[i]), out);
    }
}


RepastHPCAgentePackageReceiver::RepastHPCAgentePackageReceiver(repast::SharedContext<Agente>* agentPtr): agents(agentPtr){}

Agente * RepastHPCAgentePackageReceiver::createAgent(RepastHPCAgentePackage package){
    repast::AgentId id(package.id, package.rank, package.type, package.currentRank);
    return new Agente(id, package.prob_contagiar, package.prob_ser_contagiado, package.tipo);
}

void RepastHPCAgentePackageReceiver::updateAgent(RepastHPCAgentePackage package){
    repast::AgentId id(package.id, package.rank, package.type);
    Agente * agent = agents->getAgent(id);
    agent->set(package.currentRank, package.prob_contagiar, package.prob_ser_contagiado, package.tipo);
}



Modelo::Modelo(std::string propsFile, int argc, char** argv, boost::mpi::communicator* comm)
:	context(comm),
  	_mapa_archivo(argv[3]),
	_cant_agentes_act(0),
	_arch_salida(),
	_rank(repast::RepastProcess::instance()->rank())
	{
	props = new repast::Properties(propsFile, argc, argv, comm);
	stopAt = repast::strToInt(props->getProperty("stop.at"));
	
	initializeRandom(*props, comm);

	provider = new RepastHPCAgentePackageProvider(&context);
	receiver = new RepastHPCAgentePackageReceiver(&context);

	// Toda la parte del mapa
	std::string linea;

	int ancho, alto;

	if ( _mapa_archivo.is_open() ) {

		// Procesa la primer linea para sacar las dimensiones del mapa
		_mapa_archivo >> linea;
		
		std::regex linea_tamano("([0-9]+)x([0-9]+)");
		std::smatch match;
		std::regex_match(linea, match, linea_tamano);

		ancho = std::stoi(match[1]);
		alto = std::stoi(match[2]);
		
		if ( _rank == 0 )  std::cout << "Dimensiones del mapa: " << ancho << "," << alto << std::endl;
	} else {
		if ( _rank == 0 ) std::cout << "Archivo no abierto: " << std::endl;
	}
	
    repast::Point<double> origin(0, 0);
    repast::Point<double> extent( static_cast<double>(ancho), static_cast<double>(alto));
    
    repast::GridDimensions gd(origin, extent);

	// Nota esto estaba en 2, creemos que es el spliteo
    std::vector<int> processDims;
    processDims.push_back(repast::strToInt(props->getProperty("x.process")));
    processDims.push_back(repast::strToInt(props->getProperty("y.process")));
    
    discreteSpace = new repast::SharedDiscreteSpace<Agente, repast::StrictBorders, repast::SimpleAdder<Agente> >("AgentDiscreteSpace", gd, processDims, 1, comm);
	
    if ( _rank == 0 ) std::cout << "RANK " << _rank << " BOUNDS: " << discreteSpace->bounds().origin() << " " << discreteSpace->bounds().extents() << std::endl;
    
   	context.addProjection(discreteSpace);

	// Crea el plano
	_plano = new Plano(ancho, alto);

	// Crea el archivo de salida
	std::string _arch_salida_path = "salida" + std::to_string(_rank);
	_arch_salida.open(_arch_salida_path, std::ios::trunc);
}


Modelo::~Modelo(){
	delete props;
	delete _plano;
	delete provider;
	delete receiver;
	
	// Cierra el archivo de salida
	_arch_salida.close();
}

void Modelo::init(){

	for ( int y = 0; y < _plano->get_alto() ; y++ ) {
		
		int x = 0;
		
		for (int x = 0; x < _plano->get_ancho(); x++) {
			int tipo;
			_mapa_archivo >> tipo;
			repast::Point<int> initialLocation(x,y);

			// El process 0 tipo que encontró
			// if ( _rank == 0) std::cout << tipo << " ";
			
			// Si es una pared, le indica al mapa que ahí hay una pared
			if ( tipo == 1 ) _plano->set_pared(x, y);

			// Si es un agente y cae dentro de las coordenadas cubiertas por el process, lo crea
			else if ( tipo >= 2 && discreteSpace->dimensions().contains(initialLocation) ) {

				repast::AgentId id(_cant_agentes_act++, _rank, 0);
				id.currentRank(_rank);
				
				Agente * agent;
				agent = new Agente(id, 0.8, 0.2, tipo);
				context.addAgent(agent);
				discreteSpace->moveTo(id, initialLocation);
			}
	
		} // for columna

		//if ( _rank == 0)  std::cout << std::endl;

	} // for fila

	_mapa_archivo.close();
}

void Modelo::doSomething(){

	discreteSpace->balance();
        repast::RepastProcess::instance()->synchronizeAgentStatus<Agente, RepastHPCAgentePackage, 
             RepastHPCAgentePackageProvider, RepastHPCAgentePackageReceiver>(context, *provider, *receiver, *receiver);
    
	repast::RepastProcess::instance()->synchronizeProjectionInfo<Agente, RepastHPCAgentePackage, 
             RepastHPCAgentePackageProvider, RepastHPCAgentePackageReceiver>(context, *provider, *receiver, *receiver);

	repast::RepastProcess::instance()->synchronizeAgentStates<RepastHPCAgentePackage, 
             RepastHPCAgentePackageProvider, RepastHPCAgentePackageReceiver>(*provider, *receiver);

	int tick = repast::RepastProcess::instance()->getScheduleRunner().currentTick();
	
	// Itera sobre los agentes locales para imprimir su estado
	auto agente_it = context.localBegin();
	auto it_end = context.localEnd();

	while ( agente_it != it_end ) {
		auto agente_a_mostrar = (*agente_it);

		std::vector<int> ubicacion_ag;
		discreteSpace->getLocation( agente_a_mostrar->getId(), ubicacion_ag );
		_arch_salida << "rank=" << std::to_string(agente_a_mostrar->getId().currentRank()) << ",id=" << agente_a_mostrar->getId().id() << ",tipo=" << agente_a_mostrar->get_tipo() << ",x=" << ubicacion_ag[0] << ",y=" << ubicacion_ag[1] << ";"; // Imprime el ID del agente, coordenada x, y, y tipo

		agente_it++;
	}
	_arch_salida << std::endl;

	// Itera sobre los agentes para ver quién se contagio
	agente_it = context.localBegin();
	
	while ( agente_it != it_end ) {
		// No podemos imprimir quién fue contagiado porque se rompe
		// if ((*agente_it)->fue_contagiado() ) _arch_salida << "Me contagiaron! " << (*agente_it)->getId() << std::endl;
		agente_it++;
	}
	
	// Itera sobre los agentes locales para contagiar
	agente_it = context.localBegin();
	while ( agente_it != it_end ) {
		(*agente_it)->play(&context, discreteSpace);
		agente_it++;
	}

	// Itera sobre los agentes locales para moverse
	agente_it = context.localBegin();

	while ( agente_it != it_end ) {
		(*agente_it)->move(discreteSpace, _plano);
		agente_it++;
	}

}

void Modelo::initSchedule(repast::ScheduleRunner& runner){
	runner.scheduleEvent(1, 1, repast::Schedule::FunctorPtr(new repast::MethodFunctor<Modelo> (this, &Modelo::doSomething)));
	runner.scheduleEndEvent(repast::Schedule::FunctorPtr(new repast::MethodFunctor<Modelo> (this, &Modelo::recordResults)));
	runner.scheduleStop(stopAt);
	
}

void Modelo::recordResults(){

	if(_rank == 0){
		props->putProperty("Result","Passed");
		std::vector<std::string> keyOrder;
		keyOrder.push_back("RunNumber");
		keyOrder.push_back("stop.at");
		keyOrder.push_back("Result");
		props->writeToSVFile("./output/results.csv", keyOrder);
    }
}


	
