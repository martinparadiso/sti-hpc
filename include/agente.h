/* Demo_03_Agent.h */

#ifndef DEMO_03_AGENT
#define DEMO_03_AGENT

#include "repast_hpc/AgentId.h"
#include "repast_hpc/SharedContext.h"
#include "repast_hpc/SharedDiscreteSpace.h"

#include "plano.h"

/* Agents */
class Agente {
	
private:
    repast::AgentId _id;
    
    float _prob_contagiar;
    float _prob_ser_contagiado;
	bool _enfermo;
    bool _me_contagiaron;

public:

    Agente(repast::AgentId id, const float prob_contagiar, const float prob_ser_contagiado, const int tipo);
	
    ~Agente();
	
    /* Required Getters */
    virtual repast::AgentId& getId(){                   return _id;    }
    virtual const repast::AgentId& getId() const {      return _id;    }

    virtual float get_prob_contagiar() const { return _prob_contagiar; }
    virtual float get_prob_ser_contagiado() const { return _prob_ser_contagiado; }
	virtual int get_tipo() const { return _enfermo ? 3 : 2 ; }
    virtual bool fue_contagiado() const { return _me_contagiaron; }

    virtual void set(const int current_rank, const int prob_contagiar, const int prob_ser_contagiado, const int tipo);

    /* Actions */
    void play(repast::SharedContext<Agente>* context,
              repast::SharedDiscreteSpace<Agente, repast::StrictBorders, repast::SimpleAdder<Agente> >* space);    // Choose three other agents from the given context and see if they cooperate or not
    void move(repast::SharedDiscreteSpace<Agente, repast::StrictBorders, repast::SimpleAdder<Agente> >* space, const Plano * plano);

    // Funciones para interaccion entre agentes
    
    /**
     * Trata de contagiar a otro agente, devuelve true si logra esparcir efectivamente
     * sus bacterias en el aire
     */
    bool contagia() const;
};



//Paquete para agentes
struct RepastHPCAgentePackage {
	
public:
    int    id;
    int    rank;
    int    type;
    int    currentRank;
    double prob_contagiar;
    double prob_ser_contagiado;
	int tipo;

    // Constructores
    RepastHPCAgentePackage(); // For serialization
    RepastHPCAgentePackage(int _id, int _rank, int _type, int _currentRank, double _prob_contagiar, double _prob_ser_contagiado, int _tipo);
	
    //no necesita ser llamado explícitamente. Repast HPC se encarga de la transferencia de agentes por atrás digamos
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version){
        ar & id;
        ar & rank;
        ar & type;
        ar & currentRank;
        ar & prob_contagiar;
        ar & prob_ser_contagiado;
        ar & tipo;
    }

};


#endif