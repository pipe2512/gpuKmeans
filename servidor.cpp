#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <tuple>
#include <vector>
#include <cstdlib>
#include <random>
#include <math.h>
#include <limits>
#include <queue>
#include <zmqpp/zmqpp.hpp>
#include "timer.hh"

using namespace std;
using namespace zmqpp;

typedef vector<tuple<size_t, double>> vectorTupla;


//Funcion que se encarga de buscar y organizar los k's con su respectivo error, se realiza busqueda binaria y es magicap
void vectorK(vectorTupla &indiceK,size_t numCentroides, bool &encontrado ,double sumaDistancias, double &kAnterior, double &kSiguiente, double &kActual, size_t &kCalculo){
  //si el vector inicialmente es vacio añade el primer elemento
  if(indiceK.size() < 1){
    indiceK.push_back(make_tuple(numCentroides,sumaDistancias));
    kAnterior = -1;
    kSiguiente = -1;
    kActual = 0;
  }
  else{
    int Iarriba = indiceK.size();
    int Iabajo = 0;
    int Icentro,  indice = 0;
    while(Iabajo <= Iarriba){
      //se halla el centro entre el limite superior e inferior
      Icentro = ceil((Iarriba + Iabajo)/2);
      //se compara que el kActual que se esta examinando no se encuentre otra vez condishion de parada
      /*if(get<0>(indiceK[Icentro]) == numCentroides){
        encontrado = true;
        kAnterior = Icentro-1;
        kSiguiente = Icentro+1;
        kActual = Icentro;
        break;
      }*/
      if(abs(Iarriba - Iabajo) == 1 or abs(Iarriba - Iabajo) == 0){
        //si es 0 es porque esta posicionado bien sino es en el anterior a su posicion
        if(abs(Iarriba - Iabajo) == 0){
          indice = Iarriba;
        }
        else{
          indice = Iarriba-1;
        }
        //cuando no encuentra agrega al inicio sino agrega en el indice correspondiente
        if(get<0>(indiceK[indice]) > numCentroides){
          if(indice == 0){
            indiceK.insert(indiceK.begin(), make_tuple(numCentroides,sumaDistancias));
            kAnterior = -1;
            kSiguiente = 1;
            kActual = 0;
          }
          else{
            indiceK.insert(indiceK.begin()+(indice), make_tuple(numCentroides,sumaDistancias));
            kAnterior = indice - 1;
            kSiguiente = indice + 1;
            kActual = indice;
          }
          break;
        }
        else{
          if(abs(Iarriba - Iabajo) == 1){
            //si el indice es menor al tamaño agrega dos posiciones enseguida 
            if((indice+1) < indiceK.size()){
                if(get<0>(indiceK[indice+1]) <= numCentroides){
                  indice = indice+2;
                }else{
                  indice = indice+1;
                }
            }
          }else{
            indice = Iarriba+1;
          }
            if((Iarriba+1) < indiceK.size()){
              indiceK.insert(indiceK.begin()+(indice), make_tuple(numCentroides,sumaDistancias));
              kAnterior = indice - 1;
              kSiguiente = indice + 1;
              kActual = indice;
            }else{
              indiceK.push_back(make_tuple(numCentroides,sumaDistancias));
              kAnterior = indiceK.size() - 2;
              kSiguiente = -1;
              kActual = indiceK.size()-1;
            }
          break;
        }
      }
      else{
        if(get<0>(indiceK[Icentro]) > numCentroides){
          Iarriba = Icentro-1;
        }
        else{
          Iabajo = Icentro+1;
        }
      }
    }
  }
}


void calculaK(size_t limiteK){
  	context context;
    socket server(context,socket_type::rep);
    //socket server_recv(context,socket_type::pull);
    server.bind("tcp://*:5557");
    //server_recv.bind("tcp://*:5558");
    vector<vectorTupla> centroides;
    priority_queue<tuple<double,size_t>> colaPrioridad;
    colaPrioridad.push(make_tuple(numeric_limits<int>::max(),1));
    colaPrioridad.push(make_tuple(numeric_limits<int>::max()-2,limiteK));
    colaPrioridad.push(make_tuple(numeric_limits<int>::max()-1,ceil(limiteK/2)));
    size_t numCentroides = 0, kCalculo = 0, kMejor = 0;
    double  kSiguiente, kAnterior, kActual, kActualAnterior, kAnteriorAnterior, kSiguienteAnterior, pPendiente, sPendiente,mejorPendiente;
    double cont = 0.0, anguloAnterior = 0.0, anguloSiguiente = 0.0, anguloPrioridad = 0.0;
    bool entrada = false, parada = false, encontrado = false;
    vectorTupla indiceK;
    map<size_t, double> todos_k;
    while(parada == false)
    {
      zmqpp::message saludo;
      server.receive(saludo);
      string respuesta_solicitud;
      saludo >> respuesta_solicitud;
      double sumaDistancias = 0;
      if(respuesta_solicitud != "hola"){
        string suma_respuesta = respuesta_solicitud.substr(0,respuesta_solicitud.find(";"));
        respuesta_solicitud.erase(0,respuesta_solicitud.find(";")+1);
        sumaDistancias = atoi(suma_respuesta.c_str());
        numCentroides = atoi(respuesta_solicitud.c_str());
        //Timer t;
        vectorK(indiceK,numCentroides,encontrado,sumaDistancias, kAnterior, kSiguiente, kActual, kCalculo);

        /*for(int j = 0; j < indiceK.size(); j++){
          cout << "[ " << get<0>(indiceK[j]) << ", " << get<1>(indiceK[j]) << " ]";
        }
        cout  << endl;*/
        if(kAnterior > -1){
            //diferenciaPendiente = 0.0;
            //cout << "pPendiente "<<((double)get<1>(indiceK[kAnterior]) - (double)get<1>(indiceK[kActual]))/((double)get<0>(indiceK[kAnterior]) - (double)get<0>(indiceK[kActual]))<<endl;
            pPendiente = ((double)get<1>(indiceK[kAnterior]) - (double)get<1>(indiceK[kActual]))/((double)get<0>(indiceK[kAnterior]) - (double)get<0>(indiceK[kActual]));
        }
        if(kSiguiente < indiceK.size() and kSiguiente > -1){
            //diferenciaPendiente = 0.0;
            //cout << "sPendiente "<<((double)get<1>(indiceK[kActual]) - (double)get<1>(indiceK[kSiguiente]))/((double)get<0>(indiceK[kActual]) - (double)get<0>(indiceK[kSiguiente]))<<endl;
            sPendiente = ((double)get<1>(indiceK[kActual]) - (double)get<1>(indiceK[kSiguiente]))/((double)get<0>(indiceK[kActual]) - (double)get<0>(indiceK[kSiguiente]));
        }

        if(kAnterior > -1 and (kSiguiente < 0 or kSiguiente >= indiceK.size()) and colaPrioridad.size() == 0){
          if(kAnterior-1 > -1){
            kSiguiente = kActual;
            kActual = kAnterior;
            kAnterior = kAnterior-1;
          }else{ 
            int mitad = ceil(abs(((double)get<0>(indiceK[kActual]) - (double)get<0>(indiceK[kAnterior]))/2));
            colaPrioridad.push(make_tuple(0, (get<0>(indiceK[kActual]) - mitad)));
          }
        }
        else{
          if(kSiguiente < indiceK.size() and kSiguiente > -1 and kAnterior < 0 and colaPrioridad.size() == 0){
              int mitad = ceil(abs(((double)get<0>(indiceK[kActual]) - (double)get<0>(indiceK[kAnterior]))/2));
              colaPrioridad.push(make_tuple(0, (get<0>(indiceK[kActual]) + mitad)));
            }
          }
        if(kSiguiente < indiceK.size() and kSiguiente > -1 and kAnterior > -1){
          int mitad = ceil(abs(((double)get<0>(indiceK[kActual]) - (double)get<0>(indiceK[kAnterior]))/2));
          //size_t mitadSiguiente = ceil(abs(kSiguiente-kActual)/2);
          double diferenciaPendiente = abs(atan(pPendiente) - atan(sPendiente));
          if(diferenciaPendiente > mejorPendiente){
            cout << "la mejor pendiente es: " << diferenciaPendiente << " el mejor k por ahora es: " << get<0>(indiceK[kActual]) <<endl; 
            mejorPendiente = diferenciaPendiente;
            kMejor  = get<0>(indiceK[kActual]);
          }else{
            if(get<0>(indiceK[kAnterior]) == (get<0>(indiceK[kActual]) -1) and get<0>(indiceK[kSiguiente]) == (get<0>(indiceK[kActual]) +1)){
              parada = false;
            }
          }
          if((get<0>(indiceK[kActual]) - mitad) > 1){ 
            colaPrioridad.push(make_tuple(diferenciaPendiente, (get<0>(indiceK[kActual]) - mitad)));
          }
          if((get<0>(indiceK[kActual]) + mitad) < limiteK){
            colaPrioridad.push(make_tuple(diferenciaPendiente, (get<0>(indiceK[kActual]) + mitad)));
          }
        }  
      }
      if(indiceK.size() >= limiteK){
        parada = true;
      }
      while(entrada == false){
        if(todos_k.size() >= limiteK){
          zmqpp::message fin;
          fin << "final";
          server.send(fin);
          entrada = true;
        }
        else{
          if(colaPrioridad.size() > 0){
            numCentroides = get<1>(colaPrioridad.top());
            colaPrioridad.pop();
          }
          auto search = todos_k.find(numCentroides);
          if(search != todos_k.end())
          {
            if(colaPrioridad.size() < 1){
              numCentroides = ceil(numCentroides/2);
              if(numCentroides == 0){
                numCentroides = limiteK - cont;
                cont++;
              }
            }
          }
          else{
          entrada = true;
          todos_k.insert({numCentroides,0.0});
          string calculo_aleatorio = to_string(numCentroides);
          zmqpp::message msg;
          msg << calculo_aleatorio;
          server.send(msg);
          }
        }
      }
      entrada = false;
    }
    //server.close();
    cout << "El mejor k es: " << kMejor << endl;
}

int main(){
	calculaK(100);
}