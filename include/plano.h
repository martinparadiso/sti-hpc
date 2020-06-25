#ifndef PLANO_H
#define PLANO_H

#include <vector>

class Plano {

    private:
        const int _ancho;
        const int _alto;
        
        std::vector<bool> _plano;

    public:

    int get_alto() const { return _alto; }
    int get_ancho() const { return _ancho; }

    /**
     * Crea un plano que detalla la ubicación de las paredes
     * 
     * @param ancho Ancho del mapa
     * @param alto Alto del mapa
     */
    Plano(const int ancho, const int alto);

    /**
     * Pregunta si hay pared en una coordenada dada
     * 
     * @param x coordenada en X
     * @param y coordenada en Y
     * @return True si hay una pared, false caso contrario
     */
    bool hay_pared(const int x, const int y) const;

    /**
     * Especifica que hay una pared en una posición dada
     * 
     * @param x coordenada en X
     * @param y coordenada en Y
     */
    void set_pared(const int x, const int y);
};

#endif
