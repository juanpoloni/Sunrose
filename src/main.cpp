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
 
const char* nombresSonidos[38] = {
  "1-5 pip rapidos",       "2-3 bips largos",        "3-6 pip medios",
  "4-2 bips muy largos",   "5-8 pip veloz",           "6-Largo-corto x2",
  "7-2 cortos+largo",      "8-Largo+3 cortos",        "9-3 medios+largo",
  "10-4 bips medios",      "11-6 pip doble",          "12-Largo-corto-largo",
  "13-4 pip pausa larga",  "14-2 bips lentos",        "15-8 pip pausa doble",
  "16-3 rapidos+pausa",    "17-Crescendo",             "18-5 bips rapidos",
  "19-Pip lento x3",       "20-2 bips muy largos",    "21-Corto-largo alt.",
  "22-5 pip simetricos",   "23-Largo+3 cortos",       "24-10 pip rapidos",
  "25-Cascada desc.",       "26-Cascada asc.",         "27-Corto-largo-corto",
  "28-Largo-corto x2",     "29-4 pip medios",         "30-SOS simple",
  "31-Dos pip+largo",      "32-5 bips largos",        "33-7 pip ritmicos",
  "34-8 pip medios",       "35-3 bips lentos",        "36-Pip-largo alt.",
  "37-6 pip cuarto seg.",  "38-Secuencia musical"
};
 
// =========================================================
// ALARMAS PROGRAMADAS
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
// MOTOR DE BUZZER (non-blocking)
// =========================================================
int8_t   buzzerAlarmIdx    = -1;
uint8_t  buzzerPulsoIdx    = 0;
bool     buzzerFase        = false;
unsigned long buzzerProxMs = 0;
uint8_t  buzzerReps        = 0;
#define  BUZZER_REPS 3
 
void iniciarBuzzer(uint8_t idx) {
  buzzerAlarmIdx = idx;
  buzzerPulsoIdx = 0;
  buzzerFase     = false;
  buzzerReps     = 0;
  buzzerProxMs   = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}
 
void detenerBuzzer() {
  buzzerAlarmIdx = -1;
  digitalWrite(BUZZER_PIN, LOW);
}
 
void tickBuzzer() {
  if (buzzerAlarmIdx < 0) return;
  if (millis() < buzzerProxMs) return;
  const Pulso* pat = sonidos[buzzerAlarmIdx];
  if (pat[buzzerPulsoIdx].on == 0) {
    buzzerReps++;
    if (buzzerReps >= BUZZER_REPS) { detenerBuzzer(); return; }
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
 
int     alarmaSlider        = 0;
int     alarmaHoras         = 0;
int     alarmaMinutos       = 0;
bool    alarmaActiva        = false;
bool    alarmaDisparada     = false;
bool    alarmaApagadaManual = false;
uint8_t alarmaSonidoManual  = 0;
 
const byte Digitos[] = {
  0b1111110,0b0110000,0b1101101,0b1111001,0b0110011,
  0b1011011,0b1011111,0b1110000,0b1111111,0b1111011
};
 
int horas=0,minutos=0,segundos=0,dias=1,meses=1;
unsigned long ultSincNTP = 0;
 
void calcularHoraAlarma(int sv) {
  int t = sv * 10;
  if (t >= 1440) t = 0;
  alarmaHoras   = t / 60;
  alarmaMinutos = t % 60;
}
 
// =========================================================
// HTML: se construye pieza a pieza con resp->print()
// SIN raw strings R"()" para evitar conflictos con
// backticks y caracteres especiales de JS/HTML
// =========================================================
 
void enviarPaginaPrincipal(AsyncWebServerRequest* request) {
 
  AsyncResponseStream* resp = request->beginResponseStream("text/html");
 
  // ---- HEAD + CSS ----
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
  resp->print(".card.alarma-card{border-left-color:#ff9900}.card.alarma-card.armada{border-left-color:#ff3333}");
  resp->print(".card.prog-card{border-left-color:#4488ff}");
  resp->print(".label{font-size:12px;text-transform:uppercase;color:#888;letter-spacing:1px}");
  resp->print(".value{font-size:32px;font-weight:bold;font-family:monospace;color:#fff;margin-top:5px}");
  resp->print("input[type=range]{-webkit-appearance:none;width:100%;border-radius:8px;outline:none;height:12px;cursor:pointer}");
  resp->print("input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:25px;height:25px;border-radius:50%;background:#fff;cursor:pointer;box-shadow:0 0 5px #000}");
  resp->print("#colorSlider{background:linear-gradient(to right,red,yellow,green,cyan,blue,magenta,red)}");
  resp->print("#alarmaSlider{background:linear-gradient(to right,#1a1a2e,#ff9900)}");
  resp->print(".alarm-time-display{font-size:42px;font-weight:bold;font-family:monospace;color:#ff9900;margin:8px 0 4px;letter-spacing:4px;transition:color .3s}");
  resp->print(".alarm-time-display.armada{color:#ff4444;text-shadow:0 0 12px #f20}");
  resp->print(".alarm-sub{font-size:11px;color:#666;margin-bottom:12px}");
  resp->print(".slider-labels{display:flex;justify-content:space-between;font-size:11px;color:#555;margin-top:4px}");
  resp->print(".btn-row{display:flex;gap:8px;margin-top:12px;flex-wrap:wrap}");
  resp->print(".btn{flex:1;padding:9px 6px;border:none;border-radius:8px;font-size:12px;font-weight:bold;cursor:pointer;transition:opacity .2s,transform .1s;min-width:80px}");
  resp->print(".btn:active{transform:scale(.97)}");
  resp->print(".btn-armar{background:#ff9900;color:#111}.btn-desarmar{background:#333;color:#aaa;border:1px solid #555}.btn-apagar{background:#cc2200;color:#fff}");
  resp->print(".badge{display:inline-block;padding:3px 10px;border-radius:20px;font-size:11px;font-weight:bold;letter-spacing:1px;margin-top:6px}");
  resp->print(".badge-off{background:#2a2a2a;color:#555;border:1px solid #333}");
  resp->print(".badge-set{background:#332800;color:#ff9900;border:1px solid #ff9900}");
  resp->print(".badge-ringing{background:#3d0000;color:#ff4444;border:1px solid #f44;animation:pulso .6s infinite alternate}");
  resp->print("@keyframes pulso{from{opacity:1}to{opacity:.4}}");
  resp->print("select{background:#1a1a1a;color:#ddd;border:1px solid #444;border-radius:6px;padding:5px 8px;font-size:12px;width:100%;margin-top:6px;cursor:pointer}");
  resp->print("select:focus{outline:none;border-color:#888}");
  resp->print(".prog-table{width:100%;border-collapse:collapse;font-size:13px;margin-top:4px}");
  resp->print(".prog-table th{color:#888;font-size:11px;text-transform:uppercase;padding:4px 6px;border-bottom:1px solid #333;text-align:left;font-weight:normal;letter-spacing:1px}");
  resp->print(".prog-table td{padding:6px 6px;border-bottom:1px solid #222;vertical-align:middle;text-align:left}");
  resp->print(".prog-table tr:last-child td{border-bottom:none}.prog-table tr:hover td{background:#303030}");
  resp->print(".hora-badge{font-family:monospace;font-size:15px;font-weight:bold;color:#4af;background:#0d1a2e;padding:3px 8px;border-radius:5px}");
  resp->print(".chk{width:18px;height:18px;cursor:pointer;accent-color:#4488ff}");
  resp->print(".ringing-row td{background:#2a0000!important;animation:pulso .6s infinite alternate}");
  resp->print(".sound-sel{background:#1a1a1a;color:#ddd;border:1px solid #333;border-radius:5px;padding:3px 5px;font-size:11px;width:100%;cursor:pointer}");
  resp->print("</style></head><body><div class='container'>");
 
  // ---- TARJETA: HORA ACTUAL ----
  resp->print("<h1>&#9200; Reloj de Pared WS2812B</h1>");
  resp->print("<div class='card'>");
  resp->print("<div id='web-modo' class='label'>Cargando...</div>");
  resp->print("<div id='web-valor' class='value'>--:--</div>");
  resp->print("</div>");
 
  // ---- TARJETA: COLOR ----
  resp->print("<div class='card'><h2>Color de los Digitos</h2>");
  resp->printf("<input type='range' id='colorSlider' min='0' max='359' value='%d' oninput='enviarColor(this.value)'>", tonoElegido);
  resp->print("<div class='slider-labels'><span>0</span><span>Tono HSV</span><span>359</span></div>");
  resp->print("</div>");
 
  // ---- TARJETA: ALARMA MANUAL ----
  resp->print("<div class='card alarma-card' id='alarmaCard'>");
  resp->print("<h2>&#128276; Alarma Manual</h2>");
  resp->printf("<div id='alarmaTimeDisplay' class='alarm-time-display'>%02d:%02d</div>", alarmaHoras, alarmaMinutos);
  resp->print("<div class='alarm-sub'>Paso: 10 minutos | Maximo: 23:50</div>");
  resp->printf("<input type='range' id='alarmaSlider' min='0' max='143' step='1' value='%d' oninput='actualizarAlarmaUI(this.value)'>", alarmaSlider);
  resp->print("<div class='slider-labels'><span>00:00</span><span>Hora de alarma</span><span>23:50</span></div>");
  resp->print("<div class='label' style='margin-top:10px'>Sonido</div>");
  resp->print("<select id='sonidoManual' onchange='setSonidoManual(this.value)'>");
  for (int i = 0; i < 38; i++) {
    resp->printf("<option value='%d'%s>%s</option>", i, (i == alarmaSonidoManual ? " selected" : ""), nombresSonidos[i]);
  }
  resp->print("</select>");
  resp->print("<div id='alarmaBadge' class='badge badge-off'>DESARMADA</div>");
  resp->print("<div class='btn-row'>");
  resp->print("<button class='btn btn-armar' onclick='armarAlarma()'>Armar</button>");
  resp->print("<button class='btn btn-desarmar' onclick='desarmarAlarma()'>Desarmar</button>");
  resp->print("<button class='btn btn-apagar' id='btnApagar' style='display:none' onclick='apagarAlarma()'>Apagar</button>");
  resp->print("</div></div>");
 
  // ---- TARJETA: ALARMAS PROGRAMADAS ----
  resp->print("<div class='card prog-card'>");
  resp->print("<h2>&#128197; Alarmas Programadas</h2>");
  resp->print("<table class='prog-table'>");
  resp->print("<thead><tr><th>ON</th><th>Hora</th><th>Sonido</th><th>Estado</th></tr></thead>");
  resp->print("<tbody id='tbodyProg'></tbody>");
  resp->print("</table></div></div>");
 
  // ---- JAVASCRIPT ----
  resp->print("<script>");
 
  // Array de horarios
  resp->print("var HORARIOS=[");
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{hh:%d,mm:%d}", alarmasProg[i].hh, alarmasProg[i].mm);
    if (i < NUM_ALARMAS_PROG - 1) resp->print(",");
  }
  resp->print("];");
 
  // Array de nombres de sonidos
  resp->print("var SONIDOS=[");
  for (int i = 0; i < 38; i++) {
    resp->printf("'%s'", nombresSonidos[i]);
    if (i < 37) resp->print(",");
  }
  resp->print("];");
 
  // Estado inicial de alarmas programadas
  resp->print("var progEstado=[");
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{a:%s,s:%d,r:%s}",
      alarmasProg[i].activa    ? "true" : "false",
      alarmasProg[i].sonido,
      alarmasProg[i].disparada ? "true" : "false");
    if (i < NUM_ALARMAS_PROG - 1) resp->print(",");
  }
  resp->print("];");
 
  // Funciones JS (sin backticks, sin caracteres especiales)
  resp->print("var alarmaArmada=false,alarmaRinging=false;");
 
  resp->print("function sliderToTime(v){"
    "var m=parseInt(v)*10;"
    "var hh=Math.floor(m/60);"
    "var mm=m%60;"
    "return(hh<10?'0'+hh:hh)+':'+(mm<10?'0'+mm:mm);"
  "}");
 
  resp->print("function actualizarAlarmaUI(v){"
    "document.getElementById('alarmaTimeDisplay').innerText=sliderToTime(v);"
    "fetch('/setalarm?slider='+v);"
  "}");
 
  resp->print("function enviarColor(v){fetch('/setcolor?value='+v);}");
  resp->print("function setSonidoManual(v){fetch('/setsonidomanual?s='+v);}");
  resp->print("function armarAlarma(){alarmaArmada=true;alarmaRinging=false;fetch('/togglealarm?activa=1');refrescarEstadoAlarma();}");
  resp->print("function desarmarAlarma(){alarmaArmada=false;alarmaRinging=false;fetch('/togglealarm?activa=0');refrescarEstadoAlarma();}");
  resp->print("function apagarAlarma(){alarmaRinging=false;fetch('/stopalarm');refrescarEstadoAlarma();}");
  resp->print("function toggleProg(i,v){fetch('/settogprog?i='+i+'&a='+(v?1:0));}");
  resp->print("function setSonidoProg(i,v){fetch('/setsonidoprog?i='+i+'&s='+v);}");
 
  resp->print("function refrescarEstadoAlarma(){"
    "var card=document.getElementById('alarmaCard');"
    "var badge=document.getElementById('alarmaBadge');"
    "var timeEl=document.getElementById('alarmaTimeDisplay');"
    "var btnAp=document.getElementById('btnApagar');"
    "if(alarmaArmada){card.classList.add('armada');}else{card.classList.remove('armada');}"
    "if(alarmaRinging){timeEl.classList.add('armada');}else{timeEl.classList.remove('armada');}"
    "if(alarmaRinging){"
      "badge.className='badge badge-ringing';badge.innerText='SONANDO';btnAp.style.display='block';"
    "}else if(alarmaArmada){"
      "badge.className='badge badge-set';badge.innerText='ARMADA';btnAp.style.display='none';"
    "}else{"
      "badge.className='badge badge-off';badge.innerText='DESARMADA';btnAp.style.display='none';"
    "}"
  "}");
 
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
      "if(ringing)tr.className='ringing-row';"
      "var hStr=(h.hh<10?'0'+h.hh:h.hh)+':'+(h.mm<10?'0'+h.mm:h.mm);"
      "var selOpts='';"
      "for(var k=0;k<SONIDOS.length;k++){"
        "selOpts+='<option value='+k+(k==son?' selected':'')+'>'+SONIDOS[k]+'</option>';"
      "}"
      "var badge=ringing"
        "?'<span class=\\'badge badge-ringing\\'>SONANDO</span>'"
        ":activa?'<span class=\\'badge badge-set\\'>ARMADA</span>'"
        ":'<span class=\\'badge badge-off\\'>-</span>';"
      "tr.innerHTML="
        "'<td><input class=\\'chk\\' type=\\'checkbox\\' '+(activa?'checked':'')+' onchange=\\'toggleProg('+i+',this.checked)\\'></td>'"
        "+'<td><span class=\\'hora-badge\\'>'+hStr+'</span></td>'"
        "+'<td><select class=\\'sound-sel\\' onchange=\\'setSonidoProg('+i+',this.value)\\'>'+selOpts+'</select></td>'"
        "+'<td>'+badge+'</td>';"
      "tb.appendChild(tr);"
    "}"
  "}");
 
  resp->print("setInterval(function(){"
    "fetch('/status').then(function(r){return r.json();}).then(function(data){"
      "document.getElementById('web-modo').innerText=data.modo;"
      "document.getElementById('web-valor').innerText=data.valor;"
      "alarmaArmada=data.alarmaActiva;"
      "alarmaRinging=data.alarmaDisparada;"
      "refrescarEstadoAlarma();"
      "buildProgTable(data.prog);"
    "});"
  "},1000);");
 
  resp->print("buildProgTable(progEstado);");
  resp->printf("document.getElementById('alarmaTimeDisplay').innerText=sliderToTime(%d);", alarmaSlider);
 
  resp->print("</script></body></html>");
 
  request->send(resp);
}
 
// =========================================================
// RUTA /status
// =========================================================
void enviarStatus(AsyncWebServerRequest* request) {
  AsyncResponseStream* resp = request->beginResponseStream("application/json");
  resp->printf("{\"modo\":\"%s\",\"valor\":\"%s\",\"alarmaActiva\":%s,\"alarmaDisparada\":%s,\"prog\":[",
    stringModoWeb, stringValorWeb,
    alarmaActiva    ? "true" : "false",
    alarmaDisparada ? "true" : "false");
  for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
    resp->printf("{\"a\":%s,\"s\":%d,\"r\":%s}",
      alarmasProg[i].activa    ? "true" : "false",
      alarmasProg[i].sonido,
      alarmasProg[i].disparada ? "true" : "false");
    if (i < NUM_ALARMAS_PROG - 1) resp->print(",");
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
  calcularHoraAlarma(alarmaSlider);
  setup_wifi();
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r){ enviarPaginaPrincipal(r); });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest* r){ enviarStatus(r); });
 
  server.on("/setcolor", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("value")) { tonoElegido = req->getParam("value")->value().toInt(); actualizarPaletaColores(tonoElegido); }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/setalarm", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("slider")) {
      alarmaSlider = constrain(req->getParam("slider")->value().toInt(), 0, 143);
      calcularHoraAlarma(alarmaSlider);
      alarmaDisparada = false; alarmaApagadaManual = false;
    }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/setsonidomanual", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("s")) alarmaSonidoManual = constrain(req->getParam("s")->value().toInt(), 0, 37);
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/togglealarm", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("activa")) {
      alarmaActiva = (req->getParam("activa")->value() == "1");
      if (!alarmaActiva) { alarmaDisparada = false; alarmaApagadaManual = false; }
    }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/stopalarm", HTTP_GET, [](AsyncWebServerRequest* req){
    alarmaDisparada = false; alarmaApagadaManual = true; detenerBuzzer();
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/settogprog", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i") && req->hasParam("a")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].activa = (req->getParam("a")->value() == "1");
      if (!alarmasProg[idx].activa) { alarmasProg[idx].disparada = false; alarmasProg[idx].apagadaManual = false; }
    }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/setsonidoprog", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i") && req->hasParam("s")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].sonido = constrain(req->getParam("s")->value().toInt(), 0, 37);
    }
    req->send(200, "text/plain", "OK");
  });
 
  server.on("/stoppropalarm", HTTP_GET, [](AsyncWebServerRequest* req){
    if (req->hasParam("i")) {
      int idx = constrain(req->getParam("i")->value().toInt(), 0, NUM_ALARMAS_PROG-1);
      alarmasProg[idx].disparada = false; alarmasProg[idx].apagadaManual = true;
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
 
    // Alarma manual
    if (alarmaActiva && !alarmaApagadaManual) {
      if (horas == alarmaHoras && minutos == alarmaMinutos && !alarmaDisparada) {
        alarmaDisparada = true;
        if (buzzerAlarmIdx < 0) iniciarBuzzer(alarmaSonidoManual);
      }
    }
    if (alarmaApagadaManual && !(horas == alarmaHoras && minutos == alarmaMinutos))
      alarmaApagadaManual = false;
    if (alarmaDisparada && !(horas == alarmaHoras && minutos == alarmaMinutos))
      alarmaDisparada = false;
 
    // Alarmas programadas
    for (int i = 0; i < NUM_ALARMAS_PROG; i++) {
      if (alarmasProg[i].activa && !alarmasProg[i].apagadaManual) {
        if (horas == alarmasProg[i].hh && minutos == alarmasProg[i].mm && !alarmasProg[i].disparada) {
          alarmasProg[i].disparada = true;
          if (buzzerAlarmIdx < 0) iniciarBuzzer(alarmasProg[i].sonido);
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
    pg = horas;   sg = minutos;
    cT = ColorHora; cG = ColorGuionHora;
    cP = (segundos % 2 == 0) ? ColorPuntosHora : Apagado;
    strcpy(stringModoWeb, "MOSTRANDO: HORA");
    sprintf(stringValorWeb, "%02d:%02d", horas, minutos);
  } else {
    pg = dias;    sg = meses;
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