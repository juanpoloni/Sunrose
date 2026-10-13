#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "time.h"
#include <Adafruit_NeoPixel.h>
 
// --- Prototipos ---
void mostrarEnDisplay();
void actualizaHora();
void tickBuzzer();
 
// --- Configuracion Wi-Fi ---
const char* ssid     = "Wokwi-GUEST";
const char* password = "";
 
// --- Configuracion NTP ---
const char* ntpServer          = "ar.pool.ntp.org";
const long  gmtOffset_sec      = -10800;
const int   daylightOffset_sec = 0;
 
// --- Neopixel ---
#define DATA_PIN      5
#define LEDS_POR_SEG  17
#define SEG_POR_DIG   7
#define LEDS_POR_DIG  (LEDS_POR_SEG * SEG_POR_DIG)
#define DOS_PUNTOS    8
#define GUION         4
#define TOTAL_LEDS    ((LEDS_POR_DIG * 4) + DOS_PUNTOS + GUION)
 
Adafruit_NeoPixel TiraDeLeds(TOTAL_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);
 
// --- Buzzer ---
#define BUZZER_PIN 18
 
// =========================================================
// PATRONES DE SONIDO
// =========================================================
struct Pulso { uint16_t on; uint16_t off; };
 
#define NUM_MELODIAS 38
#define PAT_TIMBRE   100
 
// Timbre: pitido sostenido
const Pulso patTimbre[] = { {1500,500}, {0,0} };
 
// Melodia elegida en el selector (1..NUM_MELODIAS)
uint8_t melodiaElegida = 1;
 
const Pulso sonidos[38][16] = {
  /* 0 */ {{100,100},{100,100},{100,100},{100,100},{100,100},{0,0}},
  /* 1 */ {{500,200},{500,200},{500,200},{0,0}},
  /* 2 */ {{200,100},{200,100},{200,100},{200,100},{200,100},{200,100},{0,0}},
  /* 3 */ {{800,300},{800,300},{0,0}},
  /* 4 */ {{50,50},{50,50},{50,50},{50,50},{50,50},{50,50},{50,50},{50,50},{0,0}},
  /* 5 */ {{300,150},{100,150},{300,150},{100,150},{0,0}},
  /* 6 */ {{100,100},{100,100},{500,300},{0,0}},
  /* 7 */ {{500,100},{100,100},{100,100},{100,100},{0,0}},
  /* 8 */ {{200,200},{200,200},{200,200},{600,400},{0,0}},
  /* 9 */ {{400,100},{400,100},{400,100},{400,100},{0,0}},
  /*10 */ {{150,75},{150,75},{150,75},{150,75},{150,75},{150,75},{0,0}},
  /*11 */ {{600,200},{200,200},{600,200},{0,0}},
  /*12 */ {{100,200},{100,200},{100,200},{100,200},{0,0}},
  /*13 */ {{700,500},{700,500},{0,0}},
  /*14 */ {{50,100},{50,100},{50,100},{50,100},{50,100},{50,100},{50,100},{50,100},{0,0}},
  /*15 */ {{300,100},{300,100},{300,100},{300,600},{0,0}},
  /*16 */ {{100,50},{200,50},{400,50},{200,50},{100,50},{0,0}},
  /*17 */ {{400,50},{400,50},{400,50},{400,50},{400,50},{0,0}},
  /*18 */ {{200,400},{200,400},{200,400},{0,0}},
  /*19 */ {{1000,500},{1000,500},{0,0}},
  /*20 */ {{100,100},{300,100},{100,100},{300,100},{0,0}},
  /*21 */ {{250,250},{250,250},{250,250},{250,250},{250,250},{0,0}},
  /*22 */ {{500,500},{100,100},{100,100},{100,100},{0,0}},
  /*23 */ {{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{100,100},{0,0}},
  /*24 */ {{600,100},{300,100},{150,100},{75,100},{0,0}},
  /*25 */ {{75,100},{150,100},{300,100},{600,100},{0,0}},
  /*26 */ {{200,200},{400,200},{200,200},{0,0}},
  /*27 */ {{400,200},{200,200},{400,200},{200,200},{0,0}},
  /*28 */ {{300,300},{300,300},{300,300},{300,300},{0,0}},
  /*29 */ {{800,200},{200,200},{800,200},{0,0}},
  /*30 */ {{100,100},{100,100},{600,200},{100,100},{100,100},{0,0}},
  /*31 */ {{500,100},{500,100},{500,100},{500,100},{500,100},{0,0}},
  /*32 */ {{150,150},{150,150},{150,150},{150,150},{150,150},{150,150},{150,150},{0,0}},
  /*33 */ {{200,100},{200,100},{200,100},{200,100},{200,100},{200,100},{200,100},{200,100},{0,0}},
  /*34 */ {{400,400},{400,400},{400,400},{0,0}},
  /*35 */ {{100,200},{300,200},{100,200},{300,200},{100,200},{0,0}},
  /*36 */ {{250,100},{250,100},{250,100},{250,100},{250,100},{250,100},{0,0}},
  /*37 */ {{600,300},{300,150},{150,75},{600,300},{0,0}},
};
 
// =========================================================
// ALARMAS PROGRAMADAS
// sonido: 0 = Timbre, 1 = Melodia (la del selector global)
// =========================================================
#define NUM_ALARMAS_PROG 14
 
struct AlarmaProg {
  uint8_t hh;
  uint8_t mm;
  bool    activa;
  uint8_t sonido;
  bool    disparada;
  bool    apagadaManual;
};
 
AlarmaProg alarmasProg[NUM_ALARMAS_PROG] = {
  { 8,20,false,0,false,false}, { 9,20,false,0,false,false},
  { 9,30,false,0,false,false}, {10,30,false,0,false,false},
  {10,40,false,0,false,false}, {11,40,false,0,false,false},
  {11,50,false,0,false,false}, {12,40,false,0,false,false},
  {14,15,false,0,false,false}, {14,25,false,0,false,false},
  {15,20,false,0,false,false}, {15,30,false,0,false,false},
  {16,30,false,0,false,false}, {17,30,false,0,false,false},
};
 
// =========================================================
// MOTOR DE BUZZER (non-blocking) — para alarmas programadas
// =========================================================
int8_t        buzzerAlarmIdx = -1;
uint8_t       buzzerPulsoIdx = 0;
bool          buzzerFase     = false;
unsigned long buzzerProxMs   = 0;
uint8_t       buzzerReps     = 0;
uint8_t       buzzerRepsMax  = 3;
bool          buzzerPreview  = false;   // true = suena solo como aviso de cambio de melodia
#define       BUZZER_REPS    3
 
// Timbre manual: suena mientras sea true (tiene prioridad)
bool timbreManual = false;
 
void iniciarBuzzer(uint8_t idx, uint8_t reps = BUZZER_REPS, bool preview = false) {
  buzzerAlarmIdx = idx;
  buzzerPulsoIdx = 0;
  buzzerFase     = false;
  buzzerReps     = 0;
  buzzerRepsMax  = reps;
  buzzerPreview  = preview;
  buzzerProxMs   = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}
 
void detenerBuzzer() {
  buzzerAlarmIdx = -1;
  buzzerPreview  = false;
  digitalWrite(BUZZER_PIN, LOW);
}
 
void tickBuzzer() {
  // El timbre manual tiene prioridad absoluta
  if (timbreManual) {
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
  if (buzzerAlarmIdx < 0) return;
  if (millis() < buzzerProxMs) return;
  const Pulso* pat = (buzzerAlarmIdx == PAT_TIMBRE) ? patTimbre : sonidos[buzzerAlarmIdx];
  if (pat[buzzerPulsoIdx].on == 0) {
    buzzerReps++;
    if (buzzerReps >= buzzerRepsMax) { detenerBuzzer(); return; }
    buzzerPulsoIdx = 0;
    buzzerFase     = false;
  }
  if (!buzzerFase) {
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerProxMs = millis() + pat[buzzerPulsoIdx].on;
    buzzerFase   = true;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerProxMs = millis() + pat[buzzerPulsoIdx].off;
    buzzerFase   = false;
    buzzerPulsoIdx++;
  }
}
 
// =========================================================
// VARIABLES GLOBALES
// =========================================================
AsyncWebServer server(80);
 
int      tonoElegido = 120;
uint32_t ColorHora, ColorPuntosHora, ColorGuionHora;
uint32_t ColorFecha, ColorPuntosFecha, ColorGuionFecha;
const uint32_t Apagado = TiraDeLeds.Color(0,0,0);
 
char stringModoWeb[20] = "INICIALIZANDO...";
char stringValorWeb[20] = "00:00";
 
const byte Digitos[] = {
  0b1111110,0b0110000,0b1101101,0b1111001,0b0110011,
  0b1011011,0b1011111,0b1110000,0b1111111,0b1111011
};
 
int horas=0,minutos=0,segundos=0,dias=1,meses=1;
unsigned long ultSincNTP = 0;
 
// =========================================================
// PAGINA WEB
// =========================================================
void enviarPaginaPrincipal(AsyncWebServerRequest* request) {
  AsyncResponseStream* resp = request->beginResponseStream("text/html");
 
  // HEAD + CSS
  resp->print("<!DOCTYPE html><html lang='es'><head>");
  resp->print("<meta charset='UTF-8'>");
  resp->print("<meta name='viewport' content='width=device-width,initial-scale=1.0'>");
  resp->print("<title>Panel de Control - Reloj RGB</title>");
  resp->print("<style>");
  resp->print("*{box-sizing:border-box}");
  resp->print("body{font-family:'Segoe UI',sans-serif;background:#121212;color:#e0e0e0;text-align:center;margin:0;padding:20px}");
  resp->print(".container{max-width:520px;margin:auto;background:#1e1e1e;padding:28px;border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,.5)}");
  resp->print("h1{color:#00ff66;margin-bottom:22px;font-size:23px}");
  resp->print("h2{color:#aaa;font-size:13px;text-transform:uppercase;letter-spacing:2px;margin:0 0 13px 0}");
  resp->print(".card{background:#292929;padding:15px;border-radius:10px;margin-bottom:18px;border-left:5px solid #00ff66}");
  resp->print(".card.timbre-card{border-left-color:#ff9900}");
  resp->print(".card.prog-card{border-left-color:#4488ff}");
  resp->print(".label{font-size:12px;text-transform:uppercase;color:#888;letter-spacing:1px}");
  resp->print(".value{font-size:32px;font-weight:bold;font-family:monospace;color:#fff;margin-top:5px}");
  resp->print("input[type=range]{-webkit-appearance:none;width:100%;border-radius:8px;outline:none;height:12px;cursor:pointer}");
  resp->print("input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:25px;height:25px;border-radius:50%;background:#fff;cursor:pointer;box-shadow:0 0 5px #000}");
  resp->print("#colorSlider{background:linear-gradient(to right,red,yellow,green,cyan,blue,magenta,red)}");
  resp->print(".slider-labels{display:flex;justify-content:space-between;font-size:11px;color:#555;margin-top:4px}");
  // Boton timbre
  resp->print(".btn-timbre{"
    "display:block;width:100%;padding:18px;"
    "border:none;border-radius:12px;"
    "font-size:18px;font-weight:bold;letter-spacing:1px;"
    "cursor:pointer;transition:transform .1s,box-shadow .1s;"
    "background:#ff9900;color:#111;"
    "box-shadow:0 4px 12px rgba(255,153,0,.4);"
    "user-select:none;-webkit-user-select:none;"
  "}");
  resp->print(".btn-timbre:active,.btn-timbre.sonando{"
    "background:#ff3300;color:#fff;"
    "box-shadow:0 4px 16px rgba(255,50,0,.6);"
    "transform:scale(.97);"
    "animation:pulso .5s infinite alternate;"
  "}");
  // Tabla alarmas programadas
  resp->print(".prog-table{width:100%;border-collapse:collapse;font-size:13px;margin-top:4px}");
  resp->print(".prog-table th{color:#888;font-size:11px;text-transform:uppercase;padding:4px 6px;border-bottom:1px solid #333;text-align:left;font-weight:normal;letter-spacing:1px}");
  resp->print(".prog-table td{padding:6px 6px;border-bottom:1px solid #222;vertical-align:middle;text-align:left}");
  resp->print(".prog-table tr:last-child td{border-bottom:none}.prog-table tr:hover td{background:#303030}");
  resp->print(".hora-badge{font-family:monospace;font-size:15px;font-weight:bold;color:#4af;background:#0d1a2e;padding:3px 8px;border-radius:5px}");
  resp->print(".chk{width:18px;height:18px;cursor:pointer;accent-color:#4488ff}");
  resp->print(".badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:bold;letter-spacing:1px}");
  resp->print(".badge-off{background:#2a2a2a;color:#555;border:1px solid #333}");
  resp->print(".badge-set{background:#0d2200;color:#00ff66;border:1px solid #00ff66}");
  resp->print(".badge-ringing{background:#3d0000;color:#ff4444;border:1px solid #f44;animation:pulso .6s infinite alternate}");
  // Selector Timbre / Melodia (2 botones)
  resp->print(".seg{display:flex;gap:4px}");
  resp->print(".seg-b{flex:1;padding:5px 8px;border:1px solid #333;border-radius:5px;background:#1a1a1a;color:#777;font-size:11px;cursor:pointer}");
  resp->print(".seg-b.on{background:#4488ff;color:#fff;border-color:#4488ff}");
  // Selector de melodia
  resp->print(".stepper{display:flex;align-items:center;justify-content:center;gap:24px}");
  resp->print(".step-btn{width:50px;height:50px;border:none;border-radius:50%;background:#4488ff;color:#fff;font-size:26px;font-weight:bold;cursor:pointer}");
  resp->print(".step-btn:active{transform:scale(.92)}");
  resp->print("@keyframes pulso{from{opacity:1}to{opacity:.4}}");
  resp->print("</style></head><body><div class='container'>");
 
  // Tarjeta: hora actual
  resp->print("<h1>&#9200; Reloj de Pared WS2812B</h1>");
  resp->print("<div class='card'>");
  resp->print("<div id='web-modo' class='label'>Cargando...</div>");
  resp->print("<div id='web-valor' class='value'>--:--</div>");
  resp->print("</div>");
 
  // Tarjeta: color
  resp->print("<div class='card'><h2>Color de los Digitos</h2>");
  resp->printf("<input type='range' id='colorSlider' min='0' max='359' value='%d' oninput='enviarColor(this.value)'>", tonoElegido);
  resp->print("<div class='slider-labels'><span>0</span><span>Tono HSV</span><span>359</span></div>");
  resp->print("</div>");
 
  // Tarjeta: timbre manual
  resp->print("<div class='card timbre-card'>");
  resp->print("<h2>&#128276; Timbre Manual</h2>");
  resp->print("<button id='btnTimbre' class='btn-timbre'"
              " onmousedown='timbreOn()' onmouseup='timbreOff()'"
              " ontouchstart='timbreOn(event)' ontouchend='timbreOff()'>"
              "Mantener para sonar"
              "</button>");
  resp->print("</div>");
 
  // Tarjeta: selector de melodia
  resp->print("<div class='card prog-card'>");
  resp->print("<h2>&#127925; Melod&iacute;a</h2>");
  resp->printf("<div class='stepper'>"
    "<button class='step-btn' onclick='stepMelodia(-1)'>&minus;</button>"
    "<span id='melNum' class='value'>%d</span>"
    "<button class='step-btn' onclick='stepMelodia(1)'>+</button>"
    "</div></div>", melodiaElegida);
 
  // Tarjeta: alarmas programadas
  resp->print("<div class='card prog-card'>");
  resp->print("<h2>&#128197; Alarmas Programadas</h2>");
  resp->print("<table class='prog-table'>");
  resp->print("<thead><tr><th>ON</th><th>Hora</th><th>Sonido</th><th>Estado</th></tr></thead>");
  resp->print("<tbody id='tbodyProg'></tbody>");
  resp->print("</table></div>");
 
  resp->print("</div>"); // container
 
  // JavaScript
  resp->print("<script>");
 
  // Datos de horarios
  resp->print("var HORARIOS=[");
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{hh:%d,mm:%d}", alarmasProg[i].hh, alarmasProg[i].mm);
    if (i < NUM_ALARMAS_PROG-1) resp->print(",");
  }
  resp->print("];");
 
  // Estado inicial de alarmas programadas
  resp->print("var progEstado=[");
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{a:%s,s:%d,r:%s}",
      alarmasProg[i].activa    ? "true" : "false",
      alarmasProg[i].sonido,
      alarmasProg[i].disparada ? "true" : "false");
    if (i < NUM_ALARMAS_PROG-1) resp->print(",");
  }
  resp->print("];");
 
  // Funciones
  resp->print("function enviarColor(v){fetch('/setcolor?value='+v);}");
  resp->print("function toggleProg(i,v){fetch('/settogprog?i='+i+'&a='+(v?1:0));}");
  resp->print("function setSonidoProg(i,v){fetch('/setsonidoprog?i='+i+'&s='+v);}");
  resp->print("function stepMelodia(d){fetch('/stepmelodia?d='+d).then(function(r){return r.text();}).then(function(t){document.getElementById('melNum').innerText=t;});}");
 
  // Timbre manual — variable de estado para no confundir con clicks en select
  resp->print("var timbrePresionado=false;");
  resp->print("function timbreOn(e){"
    "if(e)e.preventDefault();"
    "timbrePresionado=true;"
    "document.getElementById('btnTimbre').classList.add('sonando');"
    "document.getElementById('btnTimbre').innerText='SONANDO...';"
    "fetch('/timbre?state=1');"
  "}");
  resp->print("function timbreOff(){"
    "if(!timbrePresionado)return;"
    "timbrePresionado=false;"
    "document.getElementById('btnTimbre').classList.remove('sonando');"
    "document.getElementById('btnTimbre').innerText='Mantener para sonar';"
    "fetch('/timbre?state=0');"
  "}");
  // Solo apaga si el timbre fue presionado — ignora mouseup de otros elementos
  resp->print("document.addEventListener('mouseup',timbreOff);");
  resp->print("document.addEventListener('touchend',timbreOff);");
 
  // Tabla de alarmas programadas
  resp->print("function buildProgTable(estados){"
    "var tb=document.getElementById('tbodyProg');"
    "tb.innerHTML='';"
    "for(var i=0;i<HORARIOS.length;i++){"
      "var h=HORARIOS[i];"
      "var st=estados[i]||{};"
      "var activa=st.a||false;"
      "var son=st.s||0;"
      "var ringing=st.r||false;"
      "var tr=document.createElement('tr');"
      "var hStr=(h.hh<10?'0'+h.hh:h.hh)+':'+(h.mm<10?'0'+h.mm:h.mm);"
      "var badge=ringing"
        "?'<span class=\\'badge badge-ringing\\'>SONANDO</span>'"
        ":activa?'<span class=\\'badge badge-set\\'>ARMADA</span>'"
        ":'<span class=\\'badge badge-off\\'>-</span>';"
      "tr.innerHTML="
        "'<td><input class=\\'chk\\' type=\\'checkbox\\' '+(activa?'checked':'')+' onchange=\\'toggleProg('+i+',this.checked)\\'></td>'"
        "+'<td><span class=\\'hora-badge\\'>'+hStr+'</span></td>'"
        "+'<td><div class=\\'seg\\'>'"
          "+'<button class=\\'seg-b'+(son==0?' on':'')+'\\' onclick=\\'setSonidoProg('+i+',0)\\'>Timbre</button>'"
          "+'<button class=\\'seg-b'+(son==1?' on':'')+'\\' onclick=\\'setSonidoProg('+i+',1)\\'>Melod&iacute;a</button>'"
        "+'</div></td>'"
        "+'<td>'+badge+'</td>';"
      "tb.appendChild(tr);"
    "}"
  "}");
 
  // Polling cada segundo
  resp->print("setInterval(function(){"
    "fetch('/status').then(function(r){return r.json();}).then(function(data){"
      "document.getElementById('web-modo').innerText=data.modo;"
      "document.getElementById('web-valor').innerText=data.valor;"
      "document.getElementById('melNum').innerText=data.mel;"
      "buildProgTable(data.prog);"
    "});"
  "},1000);");
 
  resp->print("buildProgTable(progEstado);");
  resp->print("</script></body></html>");
  request->send(resp);
}
 
// =========================================================
// RUTA /status
// =========================================================
void enviarStatus(AsyncWebServerRequest* request) {
  AsyncResponseStream* resp = request->beginResponseStream("application/json");
  resp->printf("{\"modo\":\"%s\",\"valor\":\"%s\",\"mel\":%d,\"prog\":[", stringModoWeb, stringValorWeb, melodiaElegida);
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{\"a\":%s,\"s\":%d,\"r\":%s}",
      alarmasProg[i].activa    ? "true" : "false",
      alarmasProg[i].sonido,
      alarmasProg[i].disparada ? "true" : "false");
    if (i < NUM_ALARMAS_PROG-1) resp->print(",");
  }
  resp->print("]}");
  request->send(resp);
}
 
// =========================================================
// COLORES
// =========================================================
void actualizarPaletaColores(int hueGrados) {
  uint32_t hueLeds      = map(hueGrados, 0, 359, 0, 65535);
  uint32_t colorUsuario = TiraDeLeds.ColorHSV(hueLeds, 255, 255);
  ColorHora        = colorUsuario;
  ColorPuntosHora  = colorUsuario;
  ColorGuionHora   = TiraDeLeds.Color(40,40,40);
  ColorFecha       = colorUsuario;
  ColorPuntosFecha = Apagado;
  ColorGuionFecha  = colorUsuario;
}
 
// =========================================================
// SETUP
// =========================================================
void setup_wifi() {
  Serial.print("Conectando");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConectado!");
  Serial.print("IP: http://"); Serial.println(WiFi.localIP());
}
 
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
 
  TiraDeLeds.begin();
  TiraDeLeds.show();
  TiraDeLeds.setBrightness(150);
 
  actualizarPaletaColores(tonoElegido);
  setup_wifi();
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){ enviarPaginaPrincipal(r); });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* r){ enviarStatus(r); });
 
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("value")) {
      tonoElegido = req->getParam("value")->value().toInt();
      actualizarPaletaColores(tonoElegido);
    }
    req->send(200, "text/plain", "OK");
  });
 
  // Timbre manual
  server.on("/timbre", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("state")) {
      timbreManual = (req->getParam("state")->value() == "1");
      if (!timbreManual) digitalWrite(BUZZER_PIN, LOW);
      Serial.printf("Timbre manual: %s\n", timbreManual ? "ON" : "OFF");
    }
    req->send(200, "text/plain", "OK");
  });
 
  // Selector de melodia (paso de a 1, con vuelta). Al cambiar suena una vez como aviso.
  server.on("/stepmelodia", HTTP_GET, [](AsyncWebServerRequest* req){
    int d = req->hasParam("d") ? req->getParam("d")->value().toInt() : 1;
    int m = (melodiaElegida - 1 + d) % NUM_MELODIAS;
    if (m < 0) m += NUM_MELODIAS;
    melodiaElegida = m + 1;
    Serial.printf("Melodia elegida: %d\n", melodiaElegida);
    // No pisa una alarma real que este sonando ni el timbre manual
    if (!timbreManual && (buzzerAlarmIdx < 0 || buzzerPreview))
      iniciarBuzzer(melodiaElegida - 1, 1, true);
    req->send(200, "text/plain", String(melodiaElegida));
  });
 
  // Alarmas programadas
  server.on("/settogprog", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i") && req->hasParam("a")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].activa = (req->getParam("a")->value() == "1");
      if (!alarmasProg[idx].activa) {
        alarmasProg[idx].disparada = false;
        alarmasProg[idx].apagadaManual = false;
      }
    }
    req->send(200, "text/plain", "OK");
  });
 
  // Tipo de sonido de la alarma: 0 = Timbre, 1 = Melodia
  server.on("/setsonidoprog", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i") && req->hasParam("s")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].sonido = (req->getParam("s")->value() == "1") ? 1 : 0;
    }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/stoppropalarm", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].disparada = false;
      alarmasProg[idx].apagadaManual = true;
    }
    detenerBuzzer();
    req->send(200, "text/plain", "OK");
  });
 
  server.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
 
// =========================================================
// LOOP
// =========================================================
void loop() {
  tickBuzzer();
 
  if (millis() - ultSincNTP >= 1000) {
    ultSincNTP = millis();
    actualizaHora();
 
    // Alarmas programadas
    for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
      if (alarmasProg[i].activa && !alarmasProg[i].apagadaManual) {
        if (horas == alarmasProg[i].hh && minutos == alarmasProg[i].mm && !alarmasProg[i].disparada) {
          alarmasProg[i].disparada = true;
          // Una alarma real pisa el aviso de cambio de melodia
          if (buzzerAlarmIdx < 0 || buzzerPreview) {
            uint8_t pat = (alarmasProg[i].sonido == 0) ? PAT_TIMBRE : (melodiaElegida - 1);
            iniciarBuzzer(pat);
          }
        }
      }
      if (alarmasProg[i].apagadaManual && !(horas == alarmasProg[i].hh && minutos == alarmasProg[i].mm))
        alarmasProg[i].apagadaManual = false;
      if (alarmasProg[i].disparada && !(horas == alarmasProg[i].hh && minutos == alarmasProg[i].mm))
        alarmasProg[i].disparada = false;
    }
 
    mostrarEnDisplay();
  }
}
 
// =========================================================
// HORA Y DISPLAY
// =========================================================
void actualizaHora() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  horas    = timeinfo.tm_hour;
  minutos  = timeinfo.tm_min;
  segundos = timeinfo.tm_sec;
  dias     = timeinfo.tm_mday;
  meses    = timeinfo.tm_mon + 1;
}
 
void muestraDigito(int valor, int ledInicial, uint32_t colorTexto) {
  byte segmentos = Digitos[valor];
  for (int s = 0; s < SEG_POR_DIG; s++) {
    bool enc = (segmentos >> (6-s)) & 0x01;
    int  lA  = ledInicial + (s * LEDS_POR_SEG);
    for (int l = 0; l < LEDS_POR_SEG; l++)
      TiraDeLeds.setPixelColor(lA+l, enc ? colorTexto : Apagado);
  }
}
 
void mostrarEnDisplay() {
  int pg, sg;
  uint32_t cT, cP, cG;
 
  if (segundos % 10 < 5) {
    pg = horas;  sg = minutos;
    cT = ColorHora; cG = ColorGuionHora;
    cP = (segundos % 2 == 0) ? ColorPuntosHora : Apagado;
    strcpy(stringModoWeb, "MOSTRANDO: HORA");
    sprintf(stringValorWeb, "%02d:%02d", horas, minutos);
  } else {
    pg = dias;   sg = meses;
    cT = ColorFecha; cP = ColorPuntosFecha; cG = ColorGuionFecha;
    strcpy(stringModoWeb, "MOSTRANDO: FECHA");
    sprintf(stringValorWeb, "%02d-%02d", dias, meses);
  }
 
  muestraDigito(pg/10, 0, cT);
  muestraDigito(pg%10, LEDS_POR_DIG, cT);
 
  int iP = LEDS_POR_DIG * 2;
  int iG = iP + DOS_PUNTOS;
  for (int i=0;i<DOS_PUNTOS;i++) TiraDeLeds.setPixelColor(iP+i, cP);
  for (int i=0;i<GUION;i++)      TiraDeLeds.setPixelColor(iG+i, cG);
 
  int d2 = iG + GUION;
  muestraDigito(sg/10, d2, cT);
  muestraDigito(sg%10, d2+LEDS_POR_DIG, cT);
 
  TiraDeLeds.show();
}
 