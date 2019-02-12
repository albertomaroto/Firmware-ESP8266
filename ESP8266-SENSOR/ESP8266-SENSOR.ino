//30.01.2019 a las 2020

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "DHT.h"

const int sensor = 12; //D6
const String TIPO = "SENSOR";

volatile bool estado = true;
volatile int numeroInt = 0;

DHT dht(sensor, DHT11);


void setup()
{
  pinMode(sensor, OUTPUT);
  //Serial.begin(115200);
  //Serial.println();
  dht.begin();
  configurar();
}

void manejarInterrupcion() {
  estado = !estado;
  numeroInt++;
}

void leerTemperatura(float *t, float *h) {
  delay(50);
  *h = dht.readHumidity();
  *t = dht.readTemperature();
}

void imprimirFormulario(WiFiClient client) {
  String formulario = "";
  formulario+="HTTP/1.1 200 OK\n";
  formulario+="Content-Type: text/html\n";
  formulario+="<html><head></head><body>\n";
  formulario+= "\n";
  formulario+="<form method=get>\n";
  formulario+="<div>\n";
  formulario+="<label for=\"name\">IP del servidor:</label>\n";
  formulario+=" <input type=\"text\" name=\"ValueIP\" />\n";
  formulario+="</div>\n";
  formulario+="<div>\n";
  formulario+="<label for=\"name\">Puerto del servidor:</label>\n";
  formulario+=" <input type=\"text\" name=\"ValuePuerto\" />\n";
  formulario+="</div>\n";
  formulario+=" <div>\n";
  formulario+="<label for=\"name\">Nombre:</label>\n";
  formulario+="<input type=\"text\" name=\"ValueID\" />\n";
  formulario+="</div>\n";
  formulario+=" <div>\n";
  formulario+="<label for=\"name\">Nombre de la red:</label>\n";
  formulario+="<input type=\"text\" name=\"ValueSSID\" />\n";
  formulario+="</div>\n";
  formulario+=" <div>\n";
  formulario+="<label for=\"name\">Clave de la red:</label>\n";
  formulario+="<input type=\"text\" name=\"ValuePASS\" />\n";
  formulario+="</div>\n";
  formulario+="<div class=\"button\">\n";
  formulario+="<button type=\"submit\">Aceptar</button>\n";
  formulario+="</div>\n";
  formulario+="</form>\n";
  formulario+="</html>";
  client.print(formulario);
}

void imprimirListo(WiFiClient client) {
  String listo = "";
  listo+="HTTP/1.1 200 OK\n";
  listo+="Content-Type: text/html\n";
  listo+="<html><head></head><body>\n";
  listo+="\n";
  listo+="Configuracion guardada satisfactoriamente\n";
  listo+="</html>\n";
  client.print(listo);
}

boolean extraerDatos(String HTTP_req, String *ip, String *puerto, String *nombre, String *ssid, String *pass) {
  int index = HTTP_req.indexOf("\n"), index2;
  String cabecera = HTTP_req.substring(0, index);
  if (cabecera.indexOf("ValueIP") > -1 && cabecera.indexOf("ValuePuerto") > -1 && cabecera.indexOf("ValueID") > -1 && cabecera.indexOf("ValueSSID") > -1 && cabecera.indexOf("ValuePASS") > -1) {//Tenemos los parámetros
    index = cabecera.indexOf("ValueIP=");
    index2 = cabecera.indexOf("&", index);
    *ip = cabecera.substring(index + 8, index2);
    
    index = cabecera.indexOf("ValuePuerto=");
    index2 = cabecera.indexOf("&", index);
    *puerto = cabecera.substring(index + 12, index2);
    
    index = cabecera.indexOf("ValueID=");
    index2 = cabecera.indexOf("&", index);
    *nombre = cabecera.substring(index + 8, index2);

    index = cabecera.indexOf("ValueSSID=");
    index2 = cabecera.indexOf("&", index);
    *ssid = cabecera.substring(index + 10, index2);

    index = cabecera.indexOf("ValuePASS=");
    index2 = cabecera.indexOf(" ", index);
    *pass = cabecera.substring(index + 10, index2);

    return true;
  }
  return false;
}

void conectar(String ip, String puerto, String nombre, String ssid, String pass, String macAdd) {
  char cssid[50], cpass[50], cid[50];
  Ticker interrupcion;
  WiFiClient client;
  int t_reconectar=5,nuevoID;
  float h, t, temperaturaMedia=0, humedadMedia=0;
  String line, peticion, respuesta, jsonID, bssid;
  String medidas;
         medidas+= "POST /medidas HTTP/1.1\n\n";
         medidas+= "{\n\"MEDIDAS\" : [\n";
  ssid.toCharArray(cssid, 50);
  pass.toCharArray(cpass, 50);

  //Levantar el wifi beaconfing
  WiFi.begin(cssid, cpass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  while(true) {
  
    if (client.connect(ip, puerto.toInt())) {
      t_reconectar=5;
      peticion = "POST /newnode HTTP/1.1\n\n";
      peticion+= "{";
      peticion+= "  \"NOMBRE\" : \"" + nombre + "\",";
      peticion+= "  \"TIPO\" : \""+TIPO+"\",";
      peticion+= "  \"MAC\" : \"" + macAdd + "\"";
      peticion+= "}\n";
      client.println(peticion);
      client.flush();
      
      while(!client.available()) {
        delay(1);
      }
      
      respuesta = client.readString();
      
      bssid = "VM-"+macAdd;
      bssid.toCharArray(cssid,50);
      
      WiFi.softAP(cssid, "12345678"); //return true if OK
      
      //susceptible de crear un procedimiento
      interrupcion.attach(5, manejarInterrupcion);

      bool ultimoEstado = false;
      while (client.connected()) {
        if (client.available()) {
          line = client.readStringUntil('\n'); //clear buffer
          //Serial.println(line);
        }
        if (ultimoEstado == estado) { //la interrupcion ha mandado leer
          leerTemperatura(&t, &h);
          if (numeroInt < 3) {
            temperaturaMedia += t;
            humedadMedia +=h;
          } else {
            temperaturaMedia += t; temperaturaMedia /= 3;
            humedadMedia +=h; humedadMedia /= 3;
            medidas += "    { \"MEDIDA\" : \"TEMPERATURA\", \"VALOR\" : "+String(temperaturaMedia)+", \"UNIDAD\" : \"ºC\" },\n"; 
            medidas += "    { \"MEDIDA\" : \"HUMEDAD\", \"VALOR\" : "+String(humedadMedia)+", \"UNIDAD\" : \"%\" }\n"; 
            medidas+="  ]\n}\n";
            numeroInt = 0;
            client.println(medidas);
            client.flush();
            medidas = "POST /medidas HTTP/1.1\n\n";
            medidas+= "{\n\"MEDIDAS\" : [\n";
            temperaturaMedia=0; 
            humedadMedia=0;
          }
          ultimoEstado = !ultimoEstado;
        } //if interrupcion
      } //while cliente connected
     //Desconectado del servidor
    client.stop();
    interrupcion.detach();
    numeroInt=0; 
    temperaturaMedia=0;
    humedadMedia=0;

    } else {
      //delay(t_reconectar*1000); /*Descomentar en entornos reales*/
      if (t_reconectar <= 80) {
        t_reconectar*=2;
      }
    }
  } //while true
}

void configurar() {

  byte mac[6];
  char apName[24], macAdd[18];
  String ip, puerto, nombre, ssid, pass;
  WiFi.macAddress(mac);
  sprintf(macAdd, "%02X:%02X:%02X:%02X:%02X:%02X%s", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  sprintf(apName, "%02X:%02X:%02X:%02X:%02X:%02X%s", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], "-config");

  WiFiServer server(80);

  String HTTP_req = ""; //para guardar la petición del cliente
  
  boolean result = WiFi.softAP(apName, "12345678"); //verificar asignacion
  
  server.begin();

  boolean configurado = false;
  while (!configurado) {
    WiFiClient client = server.available();
    boolean listo = false;
    if (client) {
      boolean blankLine = true; //variable para determinar cuándo ha terminado la petición HTTP
      while (client.connected()) {
        if (client.available()) {
          char c = client.read(); //leer caracteres de uno en uno
          HTTP_req += c;
          if (c == '\n' && blankLine) {
            listo = extraerDatos(HTTP_req, &ip, &puerto, &nombre, &ssid, &pass);
            if (!listo)
              imprimirFormulario(client);
            else {
              imprimirListo(client);
              configurado = true;
            }
            HTTP_req = "";
            break;
          }
          if (c == '\n')
            blankLine = true;
          else if (c != '\r')
            blankLine = false;
        } // if (cliente.available)
      } //While
      delay(10);
      client.stop(); //cerrar conexion
    } // if client
  } //while
  server.stop();
  conectar(ip, puerto, nombre, ssid, pass, String(macAdd));
}

void loop()
{

}
