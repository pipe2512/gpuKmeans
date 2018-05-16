#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <cstdlib>
#include <random>
#include <math.h>
#include <limits> 
#include <zmqpp/zmqpp.hpp>
#include "timer.hh"

using namespace std;
using namespace zmqpp;

typedef vector<tuple<size_t, double>> vectorTupla;
typedef unordered_map<size_t, double> tablaNorma;
typedef unordered_map<size_t, tuple<double, double>> tablaDistacias;

//Funcion que calcula las normas de los usuarios guardandolos en la tabla de normas
//NOTA: no se calcula la raiz ya que genera error en las aproximaciones
void calculoNormas(vector<vectorTupla> &vectorVectores, tablaNorma &normas){
  double calculo = 0;
  for(int j = 0; j < vectorVectores.size(); j++)
  {
    for(int i = 0; i < vectorVectores[j].size(); i++)
    {
      calculo += pow(get<1>(vectorVectores[j][i]), 2);
    }
    normas.insert({j, calculo});
    calculo = 0;
  }
}

//Funcion que calcula las normas de cada uno de los centroides retornando asi su norma
//NOTA: no se calcula la raiz ya que genera error en las aproximaciones
double normaCentroide(vectorTupla centroide){
  double calculo = 0;
  for(int i = 0; i < centroide.size(); i++)
  {
    calculo += pow(get<1>(centroide[i]), 2);
  }
  return calculo;
}

//Funcion que permite llenar un centroide en todas sus posiciones con ceros y la pelicula 
void generarDatosCero(vectorTupla &calculoCentroide){
	for(size_t i = 1; i <= 17770; i++)
	{
		calculoCentroide.push_back(make_tuple(i,0));
	}
}

//Funcion que verifica que un centroide calculado a partir de los promedios no sea 0
//NOTA: en caso de que sea 0 asigna un punto aleatorio para cambiar su posicion
void verifica(vectorTupla &calculoCentroide, tablaDistacias &distancias, vector<vectorTupla> &vectorVectores, double actual){
  double normaCalculoCentroide = normaCentroide(calculoCentroide);
  if(normaCalculoCentroide == 0)
  {
    random_device rd;
    uniform_int_distribution<int> dist(0, distancias.size());
    int indice = dist(rd);
    for(int i = 0; i < vectorVectores[indice].size(); i++)
    {
      int indicePelicula = get<0>(vectorVectores[indice][i]);
      get<1>(calculoCentroide[indicePelicula-1]) = get<1>(vectorVectores[indice][i]);
    }
    get<0>(distancias[indice]) = actual;
  }
} 

//Funcion que calcula la suma de todos los ponderados que dieron por cada centroide
double calculoError(tablaDistacias &distancias, size_t numCentroides, double &sumaDistancias){
  double error = 0.0;
  vector<double> grupos(numCentroides,0);
  vector<double> sumas(numCentroides,0);
  for(const auto recorreDistancia: distancias){
    grupos[get<0>(recorreDistancia.second)] += 1;
    sumas[get<0>(recorreDistancia.second)] += get<1>(recorreDistancia.second);
  }

  for(int i = 0; i < grupos.size(); i++)
  {
    sumaDistancias += sumas[i];
    error += (sumas[i]/grupos[i]);
  }
  return error;
}

//Funcion que genera el nuevo centroide apartir del promedio de las distancias y los usuarios
double calculaPromedio(vector<vectorTupla> &vectorVectores, vectorTupla &Centroide, tablaDistacias &distancias, double idCentroide, double normaCentorides){
	vectorTupla calculoCentroide;
	vector<size_t> cont(17770,0);
	generarDatosCero(calculoCentroide);
	double error = 0.0;
	//este for recorre las mejores distancias que tenga asignado el centroide
	for(const auto recorreDistancia: distancias){
		if(get<0>(recorreDistancia.second) == idCentroide){
			vectorTupla datos_usuario = vectorVectores[recorreDistancia.first];
      //se recorren las peliculas y se suman para sacar el promedio
      //NOTA:recordar que son vectores dispersos 
			for(size_t i = 0; i < datos_usuario.size(); i++){
				get<1>(calculoCentroide[(get<0>(datos_usuario[i]))-1]) += get<1>(datos_usuario[i]);
        cont[(get<0>(datos_usuario[i]))-1]++;
			}
		}
	}
	for(int i = 0; i < calculoCentroide.size(); i++){
      if(cont[i] != 0){
        get<1>(calculoCentroide[i]) = ceil(get<1>(calculoCentroide[i]) / cont[i]);
      }
    }
    verifica(calculoCentroide,distancias,vectorVectores,idCentroide);
    Centroide = calculoCentroide;
}

//Funcion que permite calcular la similaridad de angulosque existe entre los centroides y usuarios almacenando el menor en la tabla de distancias
void distanciasMeans(tablaDistacias &distancias, vector<vectorTupla> &usuario, vectorTupla &centroide_iterado, tablaNorma &normaUsuario, double &normaCentorides, int &centroide_actual){
  size_t m = 0, productoPunto = 0;
  double distancia = 0.0;
  for(int i = 0; i < usuario.size(); i++)
  {
    for(int j = 0; j < usuario[i].size(); j++)
    {
      m = get<0>(usuario[i][j]);
      productoPunto += get<1>(usuario[i][j]) * get<1>(centroide_iterado[m-1]);
    }
    distancia = acos(productoPunto/sqrt(normaUsuario[i] * normaCentorides));
    #pragma omp critical
    {
      if(get<1>(distancias[i]) > distancia){
        distancias[i] = make_tuple(centroide_actual,distancia);
      }
    }
    productoPunto = 0;
  }
}

//Funcion que permite reiniciar la tabla de distancias para realizar nuevos calculos
void reseteaDistancia(tablaDistacias &distancias){
  double maximo = numeric_limits<int>::max();
  for(int i = 0; i < distancias.size(); i++)
  {
    distancias[i] = make_tuple(-1,maximo);
  }
}

//Funcion donde se realiza la magia
double means(vector<vectorTupla> &vectorVectores, vector<vectorTupla> &centroides, tablaNorma &normas, tablaDistacias &distancias, size_t numCentroides){
  double error = 0.0;
  vector<double>  normaCentorides(numCentroides, 0);
  tuple<double,double> distancia;
  double errorAnterior = 0.0, sumaDistancias = 0;
  double salida = 1.0;
  int cont = 0;
  while(salida > 0.087){
    errorAnterior = error;
    error = 0.0;
    reseteaDistancia(distancias);

    #pragma omp parallel for
    //for que calcula las distancias iterando sobre cada centroide
    for(int i = 0; i < centroides.size(); i++)
    {
      normaCentorides[i] = normaCentroide(centroides[i]);
      distanciasMeans(distancias,vectorVectores,centroides[i],normas,normaCentorides[i],i);
    }
    #pragma omp parallel for
    //for  que calcula los promedios iterando sobre cada centroide
    for(int i = 0; i < centroides.size(); i++){
      //como es una suma no hay problemas con la condicion de carrera
      calculaPromedio(vectorVectores,centroides[i],distancias,i,normaCentorides[i]);
    }
    sumaDistancias = 0;
    error = calculoError(distancias,numCentroides, sumaDistancias);
    salida = abs(error - errorAnterior);
  }
  return sumaDistancias;
}

//Funcion que genera los centroides tomando de manera aleatoria los puntos existentes
void generarCentroidePuntos(vector<vectorTupla> &centroides, size_t numCentroides, vector<vectorTupla> &vectorVectores){
  random_device rd;
  uniform_int_distribution<int> dist(0, vectorVectores.size() - 1);
  for(size_t i = 0; i < numCentroides; i++){
    size_t indice = dist(rd);
    vectorTupla calificaciones;
    generarDatosCero(calificaciones);
    for(size_t j = 0; j < vectorVectores[indice].size(); j++){
      int indicePelicula = get<0>(vectorVectores[indice][j]);
      get<1>(calificaciones[indicePelicula-1]) = get<1>(vectorVectores[indice][i]);
    }
    centroides.push_back(calificaciones);
  }

}

//Funcion donde se lee el archivo y se crean de manera inicial los datos para trabajar
void lecturaArchivo(unordered_map<size_t, size_t> &datos, tablaDistacias &distancias, vector<vectorTupla> &vectorVectores){
  ifstream archivo("netflix/combined_data_1.txt");
  //ifstream archivo("netflix/prueba.txt");
  char linea[256];
  string line;
  vectorTupla argumentos;
  size_t idPelicula = 0;
  size_t key = 0;
  double calificacion = 0;
  while(getline(archivo,line))
  {
    int posicionCaracter = line.find(":");
    if(posicionCaracter != string::npos){
      idPelicula = atoi(line.substr(0, posicionCaracter).c_str());
    }
    else{
        for(int j = 0; j < 2; j++){
          posicionCaracter = line.find(",");
          if(j == 0){
            key = atoi(line.substr(0, posicionCaracter).c_str());
            line.erase(0, posicionCaracter + 1);
          }
          else{
            calificacion = atoi(line.substr(0, posicionCaracter).c_str());
            auto search = datos.find(key);
            if(search != datos.end())
            {
              vectorVectores[search->second].push_back(make_tuple(idPelicula, calificacion));

            }
            else
            {
              argumentos.push_back(make_tuple(idPelicula, calificacion));
              vectorVectores.push_back(argumentos);
              datos.insert({key, (vectorVectores.size() - 1)});
              double maximo = numeric_limits<int>::max();
              distancias.insert({(vectorVectores.size() - 1), make_tuple(-1, maximo)});
              argumentos.clear();
            }
          }
        }
    }

  }
  archivo.close();
}

//Funcion principal encargada de ejecutar cada coshita
int main(){
  unordered_map<size_t, size_t> datos;
  vector<vectorTupla> centroides;
  tablaDistacias distancias;
  ////////////////////////////////////////////////////
  /*string ipserver="localhost";
  context context;
  socket cliente(context,socket_type::req);

  //Conexion Server

  const string conexionserver = "tcp://"+ipserver+":4000";


  cliente.connect(conexionserver);*/

  vector<vectorTupla> vectorVectores;
  tablaNorma normas;
  lecturaArchivo(datos, distancias, vectorVectores);
  cout << "Datos cargados..." << endl;
  calculoNormas(vectorVectores, normas);
  cout << "Normas calculadas..." << endl;
  context context;
  socket cliente(context,socket_type::req);
  cliente.connect("tcp://localhost:5557");
  zmqpp::message msg;
  msg << "hola";
  cliente.send(msg);
  /*socket cliente_send(context,socket_type::push);
  cliente_send.connect("tcp://localhost:5558");*/
  string x;
  bool final = false;
  while(final == false){
    zmqpp::message msg2;
    cliente.receive(msg2);
    msg2 >> x;
    if(x == "final"){
      final = true;
    }
    else{
    cout<<"K recibido es: "<< atoi(x.c_str()) <<endl;
    size_t numCentroides = atoi(x.c_str());
    //cout << "ingrese el número de centroides: ";
    //cin >> numCentroides;
    generarCentroidePuntos(centroides, numCentroides, vectorVectores);
    //Timer t;
    zmqpp::message resultado;
    string m = to_string(means(vectorVectores, centroides, normas, distancias, numCentroides));
    resultado << m+";"+x;
    centroides.clear();
    cliente.send(resultado);
    }
  }
  cout << "Ya termine mi labor" << endl;
}
