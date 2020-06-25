#include "plano.h"

#include <vector>

Plano::Plano(const int ancho, const int alto) : _ancho(ancho), _alto(alto), _plano(ancho * alto, false) {
    
}


bool Plano::hay_pared(const int x, const int y) const {
    return _plano[y * _ancho + x];
}


void Plano::set_pared(const int x, const int y){
    _plano[y * _ancho + x] = true;
}
