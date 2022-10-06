// Importação de bibliotecas:
#include <Adafruit_Sensor.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>
#include <Stepper.h>
#include <WiFi.h>
#include <time.h>
#include <DHT.h>



// Pinagem digital:
#define PINO_UMIDIFICADOR 22
#define PINO_RESFRIAMENTO 5
#define PINO_AQUECIMENTO 18
#define PINO_ILUMINACAO 12
#define PINO_IRRIGACAO 19
#define PINO_DHT 4



// Pinagem analógica:
#define PINO_FC28 34
#define PINO_LDR A0



// Pinagem do motor:
#define PINO_JANELA_1 35
#define PINO_JANELA_2 32
#define PINO_JANELA_3 33
#define PINO_JANELA_4 25



// Conexão banco de dados em nuvem - Firebase, host e auth:
#define FIREBASE_AUTENTICADOR "tcI7b3wI51dzLwN7Bhc5bDMHHJNltEDK7lJZo79r"
#define FIREBASE_HOST "estufa-automatica-default-rtdb.firebaseio.com"

FirebaseData firebaseData;



// Conexão wi-fi, SSID e senha da rede:
#define WIFI_IDENTIFICADOR "ESTUFA"
#define WIFI_SENHA "lucas123"



// Captura de data e hora locais:
#define SERVIDOR_NTP "pool.ntp.br"

const int daylightOffset_sec = -5400;
const long gmtOffset_sec = -5400;



// Modelo de sensor DHT:
#define TIPO_DHT DHT22



// Criação do objeto DHT com parâmetros necessários, pinagem e modelo:
DHT dht(PINO_DHT, TIPO_DHT);



// Variáveis globais:
int auxiliar_generica_de_controle = 5;
int controle_dia_da_semana = 0;
int controle_de_irrigacao = 0;
int controle_da_janela = 0;




// Variáveis de controle do motor:
const int angulacao_da_engrenagem = 64;
Stepper motor_janela(angulacao_da_engrenagem, PINO_JANELA_1, PINO_JANELA_3, PINO_JANELA_2, PINO_JANELA_4);



// Código SETUP - lido apenas 1x:
void setup() {
    // Entradas:
    pinMode(PINO_FC28, INPUT);
    pinMode(PINO_LDR, INPUT);
    pinMode(PINO_DHT, INPUT);

    // Saídas:
    pinMode(PINO_UMIDIFICADOR, OUTPUT);
    pinMode(PINO_RESFRIAMENTO, OUTPUT);
    pinMode(PINO_AQUECIMENTO, OUTPUT);
    pinMode(PINO_ILUMINACAO, OUTPUT);
    pinMode(PINO_IRRIGACAO, OUTPUT);
    pinMode(PINO_JANELA_1, OUTPUT);
    pinMode(PINO_JANELA_2, OUTPUT);
    pinMode(PINO_JANELA_3, OUTPUT);
    pinMode(PINO_JANELA_4, OUTPUT);
    

    // Velocidade do monitor serial:
    Serial.begin(9600);


    // Atraso de inicialização --> conexão:
    delay(2500);


    // Conexão wi-fi:
    WiFi.begin(WIFI_IDENTIFICADOR, WIFI_SENHA);
    Serial.print("Conectando-se à rede wi-fi..");

    while(WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(2500);
    }

    // Identificação do endereço IP local conectado:
    Serial.println();
    Serial.print("Rede wi-fi conectada, endereço IP: ");
    Serial.println(WiFi.localIP());


    // Inicialização e captura de data e hora:
    configTime(gmtOffset_sec, daylightOffset_sec, SERVIDOR_NTP);


    // Conexão banco de dados em nuvem - Firebase:
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTENTICADOR);
    Firebase.reconnectWiFi(true);


    // Setando velocidade do motor em rotações por minuto:
    motor_janela.setSpeed(500);


    // Inicialização DHT:
    dht.begin();
}



// Código LOOP - lido repetidas vezes:
void loop() {
    // Variáveis de medições de umidade e temperatura:
    float umidade = dht.readHumidity();
    float temperatura = dht.readTemperature();
    
    // Variável de medição de luminosidade:
    int luminosidade = analogRead(PINO_LDR);

    // Variáveis de medição de umidade do solo:
    int umidade_do_solo = analogRead(PINO_FC28);
    float umidade_do_solo_porcentagem = map(umidade_do_solo, 4095, 1600, 0, 100);


    // Checagem de falhas no DHT22:
    if(isnan(umidade) || isnan(temperatura)) {
        Serial.println(F("[!] Erro ao capturar os dados do sensor DHT."));
        return;
    }



    Serial.println("");
    Serial.println("--------------- ------- - ------- ---------------");
    Serial.println("");



    // Variáveis de captura do servidor NTP:
    char hora[3];
    char minuto[3];
    char dia_da_semana[10];

    // Captura de data de hora:
    struct tm informacoes_do_tempo;

    if(!getLocalTime(&informacoes_do_tempo)) {
        Serial.println("[!] Erro ao capturar os dados do servidor NTP.");
        return;
    }

    // Data e hora formatados:
    strftime(hora, 3, "%H", &informacoes_do_tempo);
    strftime(minuto, 3, "%M", &informacoes_do_tempo);
    strftime(dia_da_semana, 10, "%A", &informacoes_do_tempo);


    // Assimilação dos valores de iluminação - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/Iluminacao")) {
        String iluminacao_binario = firebaseData.stringData();

        if(iluminacao_binario.toInt() == 0) {
            digitalWrite(PINO_ILUMINACAO, LOW);
            Serial.println("• Iluminação desligada.");
        } else {
            digitalWrite(PINO_ILUMINACAO, HIGH);
            Serial.println("• Iluminação ligada.");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Iluminação - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Assimilação dos valores de resfriamento - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/CoolerResfriamento")) {
        String resfriamento_binario = firebaseData.stringData();

        if(resfriamento_binario.toInt() == 0) {
            digitalWrite(PINO_RESFRIAMENTO, LOW);
            Serial.println("• Resfriamento desligado.");
        } else {
            digitalWrite(PINO_RESFRIAMENTO, HIGH);
            Serial.println("• Resfriamento ligado.");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Resfriamento - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Assimilação dos valores de aquecimento - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/PlacaAquecimento")) {
        String aquecimento_binario = firebaseData.stringData();

        if(aquecimento_binario.toInt() == 0) {
            digitalWrite(PINO_AQUECIMENTO, LOW);
            Serial.println("• Aquecimento desligado.");
        } else {
            digitalWrite(PINO_AQUECIMENTO, HIGH);
            Serial.println("• Aquecimento ligado.");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Aquecimento - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Assimilação dos valores de umidificação - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/Exaustor")) {
        String exaustao_binario = firebaseData.stringData();

        if(exaustao_binario.toInt() == 0) {
            digitalWrite(PINO_UMIDIFICADOR, LOW);
            Serial.println("• Umidificação desligada.");
        } else {
            digitalWrite(PINO_UMIDIFICADOR, HIGH);
            Serial.println("• Umidificação ligada.");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Umidificação - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Assimilação dos valores de desumidificação - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/Janelas")) {
        String desumidificacao_binario = firebaseData.stringData();

        if(desumidificacao_binario.toInt() == 0) {
            Serial.println("• Desumidificação desligada [janela fechada].");
        } else {
            Serial.println("• Desumidificação ligada [janela aberta].");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Desumidificação - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Assimilação dos valores de irrigação - 0 ou 1:
    if(Firebase.getString(firebaseData, "/Atuadores/Bomba")) {
        String irrigacao_binario = firebaseData.stringData();

        if(irrigacao_binario.toInt() == 0) {
            digitalWrite(PINO_IRRIGACAO, LOW);
            Serial.println("• Irrigação desligada.");
        } else {
            digitalWrite(PINO_IRRIGACAO, HIGH);
            Serial.println("• Irrigação ligada.");
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Irrigação - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }



    // Atualização dos valores de umidade, DHT --> banco de dados em nuvem:
    if(Firebase.get(firebaseData, "VariaveisMedidas/UmidadeAr")) {
        Firebase.set(firebaseData, "/VariaveisMedidas/UmidadeAr", umidade);
    } else {
        Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Umidade do Ar - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Atualização dos valores de temperatura, DHT --> banco de dados em nuvem:
    if(Firebase.get(firebaseData, "/VariaveisMedidas/Temperatura")) {
        Firebase.set(firebaseData, "/VariaveisMedidas/Temperatura", temperatura);
    } else {
        Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Temperatura - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Atualização dos valores de luminosidade, LDR --> banco de dados em nuvem:
    if(Firebase.get(firebaseData, "/VariaveisMedidas/Luminosidade")) {
        Firebase.set(firebaseData, "/VariaveisMedidas/Luminosidade", luminosidade);
    } else {
        Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Luminosidade - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }


    // Atualização dos valores de umidade do solo, FC-28 --> banco de dados em nuvem:
    if(Firebase.get(firebaseData, "/VariaveisMedidas/UmidadeSolo")) {
        Firebase.set(firebaseData, "/VariaveisMedidas/UmidadeSolo", umidade_do_solo_porcentagem);
    } else {
        Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Umidade do Solo - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }



    Serial.println("");
    Serial.println("--------------- ------- - ------- ---------------");
    Serial.println("");



    // Assimilação dos valores de controle - 0 [manual] ou 1 [automático]:
    if(Firebase.getString(firebaseData, "/Variaveis/Controle")) {
        String controle_binario = firebaseData.stringData();

        if(controle_binario.toInt() == 0) {
            Serial.println("• Controle manual.");
        } else {
            // Lógica de acionamento de iluminação artificial:
            if(Firebase.getString(firebaseData, "/Variaveis/LuminosidadeMinima")) {
                String luminosidade_minima = firebaseData.stringData();

                Serial.print("• Luminosidade mínima: ");
                Serial.print(luminosidade_minima);
                Serial.println(" Lux");

                // Controle do atuador - lâmpada incandescente:
                if(luminosidade < luminosidade_minima.toInt()) {
                    Firebase.set(firebaseData, "/Atuadores/Iluminacao", "1");
                } else {
                    Firebase.set(firebaseData, "/Atuadores/Iluminacao", "0");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Lâmpada Incandescente - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de acionamento de resfriamento:
            if(Firebase.getString(firebaseData, "/Variaveis/TemperaturaMaxima")) {
                String temperatura_maxima = firebaseData.stringData();

                Serial.print("• Temperatura máxima: ");
                Serial.print(temperatura_maxima);
                Serial.println(" °C");

                // Controle do atuador - coolers:
                if(temperatura > temperatura_maxima.toInt()) {
                    Firebase.set(firebaseData, "/Atuadores/CoolerResfriamento", "1");
                } else {
                    Firebase.set(firebaseData, "/Atuadores/CoolerResfriamento", "0");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Coolers - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de acionamento de aquecimento:
            if(Firebase.getString(firebaseData, "/Variaveis/TemperaturaMinima")) {
                String temperatura_minima = firebaseData.stringData();

                Serial.print("• Temperatura mínima: ");
                Serial.print(temperatura_minima);
                Serial.println(" °C");

                // Controle do atuador - placa de porcelana:
                if(temperatura < temperatura_minima.toInt()) {
                    Firebase.set(firebaseData, "/Atuadores/PlacaAquecimento", "1");
                } else {
                    Firebase.set(firebaseData, "/Atuadores/PlacaAquecimento", "0");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Placa de Porcelana - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de acionamento de desumidificação:
            if(Firebase.getString(firebaseData, "/Variaveis/UmidadeMaximaAr")) {
                String umidade_maxima_do_ar = firebaseData.stringData();

                Serial.print("• Umidade máxima do ar: ");
                Serial.print(umidade_maxima_do_ar);
                Serial.println(" %");

                // Controle do atuador - janela:
                if((umidade >= umidade_maxima_do_ar.toInt()) and (controle_da_janela == 0)) {
                    Firebase.set(firebaseData, "/Atuadores/Janelas", "1");
                    motor_janela.step(angulacao_da_engrenagem*32*7);
                    controle_da_janela = 1;
                } else {
                    if((umidade < (umidade_maxima_do_ar.toInt() - auxiliar_generica_de_controle)) and (controle_da_janela == 1)) {
                        Firebase.set(firebaseData, "/Atuadores/Janelas", "0");
                        motor_janela.step(-angulacao_da_engrenagem*32*4);
                        controle_da_janela = 0;
                    }
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Janela - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de acionamento de umidificação:
            if(Firebase.getString(firebaseData, "/Variaveis/UmidadeMinimaAr")) {
                String umidade_minima_do_ar = firebaseData.stringData();

                Serial.print("• Umidade mínima do ar: ");
                Serial.print(umidade_minima_do_ar);
                Serial.println(" %");

                // Controle do atuador - piezo elétrico e bomba de umidificação:
                if(umidade < umidade_minima_do_ar.toInt()) {
                    Firebase.set(firebaseData, "/Atuadores/Exaustor", "1");
                } else {
                    Firebase.set(firebaseData, "/Atuadores/Exaustor", "0");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Piezo Elétrico - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }



            // Variáveis locais de aferição de data e hora [hh:mm]:
            int resultado_hora_minima;
            int resultado_hora_maxima;

            int resultado_minuto_maximo;
            int resultado_minuto_minimo;


            // Lógica de comparação para "hora mínima":
            if(Firebase.getString(firebaseData, "/Variaveis/HoraMinima")) {
                String hora_minima = firebaseData.stringData();

                // Comparação, hora atual vs. hora mínima:
                resultado_hora_minima = strcmp(hora, hora_minima.c_str());
            } else {
                Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Hora de Início - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de comparação para "minuto mínimo":
            if(Firebase.getString(firebaseData, "/Variaveis/MinutoMinimo")) {
                String minuto_minimo = firebaseData.stringData();

                // Comparação, minuto atual vs. minuto mínimo:
                resultado_minuto_minimo = strcmp(minuto, minuto_minimo.c_str());
            } else {
                Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Minuto de Início - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de comparação para "hora máxima":
            if(Firebase.getString(firebaseData, "/Variaveis/HoraMaxima")) {
                String hora_maxima = firebaseData.stringData();

                // Comparação, hora atual vs. hora máxima:
                resultado_hora_maxima = strcmp(hora, hora_maxima.c_str());
            } else {
                Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Hora de Término - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de comparação para "minuto máximo":
            if(Firebase.getString(firebaseData, "/Variaveis/MinutoMaximo")) {
                String minuto_maximo = firebaseData.stringData();

                // Comparação, minuto atual vs. minuto máximo:
                resultado_minuto_maximo = strcmp(minuto, minuto_maximo.c_str());
            } else {
                Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Minuto de Término - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Horário de início - temos que, para o resultado igual a 0 [true] --> acionamento da irrigação:
            if((resultado_hora_minima == 0) and (resultado_minuto_minimo == 0)) {
                // Assimilação dos valores de irrigação - 0 a 7:
                if(Firebase.getString(firebaseData, "/Variaveis/DiaDaSemana")) {
                    String controle_dia_da_semana = firebaseData.stringData();

                    // Irrigação intervalada entre os dias da semana:
                    switch (controle_dia_da_semana.toInt()) {
                        case 1:
                            if(strcmp(dia_da_semana, "Wednesday") == 0) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 1x por semana.");
                            }
                            break;

                        case 2:
                            if((strcmp(dia_da_semana, "Tuesday") == 0) or (strcmp(dia_da_semana, "Thursday") == 0)) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 2x por semana.");
                            }
                            break;

                        case 3:
                            if((strcmp(dia_da_semana, "Monday") == 0) or (strcmp(dia_da_semana, "Wednesday") == 0) or (strcmp(dia_da_semana, "Friday") == 0)) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 3x por semana.");
                            }
                            break;
                        
                        case 4:
                            if((strcmp(dia_da_semana, "Monday") == 0) or (strcmp(dia_da_semana, "Tuesday") == 0) or (strcmp(dia_da_semana, "Thursday") == 0) or (strcmp(dia_da_semana, "Friday") == 0)) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 4x por semana.");
                            }
                            break;
                        
                        case 5:
                            if((strcmp(dia_da_semana, "Sunday") == 0) or (strcmp(dia_da_semana, "Monday") == 0) or (strcmp(dia_da_semana, "Wednesday") == 0) or (strcmp(dia_da_semana, "Friday") == 0) or (strcmp(dia_da_semana, "Saturday") == 0)) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 5x por semana.");
                            }
                            break;
                        
                        case 6:
                            if((strcmp(dia_da_semana, "Sunday") == 0) or (strcmp(dia_da_semana, "Monday") == 0) or (strcmp(dia_da_semana, "Tuesday") == 0) or (strcmp(dia_da_semana, "Thursday") == 0) or (strcmp(dia_da_semana, "Friday") == 0) or (strcmp(dia_da_semana, "Saturday") == 0)) {
                                controle_de_irrigacao = 1;
                                Serial.println("• Irrigação 6x por semana.");
                            }
                            break;
                        
                        case 7:
                            controle_de_irrigacao = 1;
                            Serial.println("• Irrigação 7x por semana.");
                            break;
                    
                        default:
                            Serial.print("• Qtd. de irrigações por semana não definida - Necessário verificação do parâmetro.");
                            controle_de_irrigacao = 0;
                            break;
                    }
                } else {
                    Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Dia da Semana - ");
                    Serial.print(firebaseData.errorReason());
                    Serial.println(".");
                }
            }

            // Horário de término - temos que, para o resultado igual a 0 [true] --> desacionamento da irrigação:
            if((resultado_hora_maxima == 0) and (resultado_minuto_maximo == 0)) {
                controle_de_irrigacao = 0;
            }



            // Lógica de acionamento de irrigação:
            if(Firebase.getString(firebaseData, "/Variaveis/UmidadeMinimaSolo")) {
                String umidade_minima_do_solo = firebaseData.stringData();

                Serial.print("• Umidade mínima do solo: ");
                Serial.print(umidade_minima_do_solo);
                Serial.println(" %");

                // Controle do atuador - bomba:
                if((umidade_do_solo_porcentagem <= umidade_minima_do_solo.toInt()) and (controle_de_irrigacao == 1)) {
                    Firebase.set(firebaseData, "/Atuadores/Bomba", "1");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Bomba d'água - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }


            // Lógica de desacionamento de irrigação:
            if(Firebase.getString(firebaseData, "/Variaveis/UmidadeMaximaSolo")) {
                String umidade_maxima_do_solo = firebaseData.stringData();

                Serial.print("• Umidade máxima do solo: ");
                Serial.print(umidade_maxima_do_solo);
                Serial.println(" %");

                // Controle do atuador - bomba:
                if((umidade_do_solo_porcentagem > umidade_maxima_do_solo.toInt()) or (controle_de_irrigacao == 0)) {
                    Firebase.set(firebaseData, "/Atuadores/Bomba", "0");
                }
            } else {
                Serial.print("[!] Erro ao enviar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Bomba d'água - ");
                Serial.print(firebaseData.errorReason());
                Serial.println(".");
            }
        }
    } else {
        Serial.print("[!] Erro ao capturar os dados em nuvem: [LATÊNCIA FIREBASE][ttl] - Controle [manual][automático] - ");
        Serial.print(firebaseData.errorReason());
        Serial.println(".");
    }



    Serial.println("");
    Serial.println("--------------- ------- - ------- ---------------");
    Serial.println("");



    // Data e hora atuais:
    Serial.print("[?] Estufa Automática | Horário corrente: ");
    Serial.print(hora);
    Serial.print(":");
    Serial.print(minuto);
    Serial.print(" - ");


    // Dia da semana traduzido:
    if(strcmp(dia_da_semana, "Sunday") == 0) {
        Serial.println("Domingo");
    } else {
        if(strcmp(dia_da_semana, "Monday") == 0) {
            Serial.println("Segunda");
        } else {
            if(strcmp(dia_da_semana, "Tuesday") == 0) {
                Serial.println("Terça");
            } else {
                if(strcmp(dia_da_semana, "Wednesday") == 0) {
                    Serial.println("Quarta");
                } else {
                    if(strcmp(dia_da_semana, "Thursday") == 0) {
                        Serial.println("Quinta");
                    } else {
                        if(strcmp(dia_da_semana, "Friday") == 0) {
                            Serial.println("Sexta");
                        } else {
                            Serial.println("Sábado");
                        }
                    }
                }
            }
        }
    }



    // Delay de atualização do laço - 50 ms:
    delay(50);
}
