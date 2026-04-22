# Nixie Clock Ultra – Bedienungsanleitung

## Übersicht

Die Nixie Clock Ultra ist eine Röhrenuhr auf Basis des ESP32-S3 mit 6 Nixie-Röhren, 10 NeoPixel-LEDs, einem Web-Interface zur Fernbedienung sowie einem IR-Empfänger für die Steuerung per Infrarot-Fernbedienung.

---

## Inbetriebnahme

1. Gerät mit Spannung versorgen
2. Die Nixie-Röhren starten mit einem sanften Fade-In
3. Die Uhr zeigt sofort die in der RTC gespeicherte Zeit an

> **Sicherheitshinweis:** Die Nixie-Hochspannung beträgt 170–180 V DC. Vor Arbeiten an der Schaltung unbedingt die Kondensatoren entladen. Nie unter Spannung an der Schaltung arbeiten.

---

## Tasten

Das Gerät besitzt vier Tasten:

| Taste   | Funktion                                      |
|---------|-----------------------------------------------|
| SET     | Einstellmodus starten / nächste Stelle wählen |
| UP      | Wert erhöhen (mit Auto-Repeat bei Dauerdruck) |
| DOWN    | Wert verringern (mit Auto-Repeat bei Dauerdruck) |
| LIGHT   | Helligkeit umschalten (4 Stufen)              |

---

## IR-Fernbedienung

Die Uhr kann mit einer beliebigen Infrarot-Fernbedienung gesteuert werden. Jede Taste der Fernbedienung kann einer Funktion zugewiesen werden (siehe [Fernbedienung anlernen](#fernbedienung-anlernen) im Web-Interface).

### Verfügbare IR-Funktionen

| Funktion            | Beschreibung                                              |
|---------------------|-----------------------------------------------------------|
| SET                 | Einstellmodus starten / nächste Stelle / Zeit speichern  |
| UP                  | Wert erhöhen im Einstellmodus                            |
| DOWN                | Wert verringern im Einstellmodus                         |
| BRIGHTNESS          | Helligkeit umschalten (4 Stufen)                         |
| ANIM_NEXT           | Nächsten Animationsmodus aktivieren                      |
| SLOT                | Slot-Machine-Animation auslösen                          |
| POWER_SAVE_TOGGLE   | Power-Save-Modus ein- oder ausschalten                   |

Nicht belegte Funktionen werden ignoriert. Repeat-Codes (Auto-Repeat der Fernbedienung) werden beim Empfang herausgefiltert.

---

## Zeit einstellen (Tasten)

1. **SET** drücken → Stunden blinken
2. Mit **UP** / **DOWN** Stunden einstellen
3. **SET** drücken → Minuten blinken
4. Mit **UP** / **DOWN** Minuten einstellen
5. **SET** drücken → Sekunden blinken
6. Mit **UP** / **DOWN** Sekunden einstellen
7. **SET** drücken → Zeit wird gespeichert, Normalanzeige

Die blinkende Stelle zeigt an, welcher Wert gerade eingestellt wird. Nach **15 Sekunden** ohne Eingabe wird der Einstellmodus automatisch beendet und die Zeit gespeichert.

---

## Helligkeit (Taste LIGHT)

Jeder Druck auf **LIGHT** schaltet zur nächsten Helligkeitsstufe:

| Stufe | Beschreibung  |
|-------|---------------|
| 1     | Sehr dim      |
| 2     | Dim           |
| 3     | Hell          |
| 4     | Voll (Standard) |

Die gewählte Stufe wird dauerhaft gespeichert.

---

## Power-Save

Nach **120 Sekunden** ohne Tastendruck dimmt die NeoPixel-Beleuchtung automatisch auf ein Viertel der eingestellten Helligkeit. Jeder Tasten- oder Fernbedienungsdruck weckt die Uhr wieder auf volle Helligkeit.

Der Power-Save-Modus kann dauerhaft deaktiviert werden – entweder über den **Power-Save-Toggle** im Web-Interface (Karte „Helligkeit & Animation") oder über die zugewiesene IR-Taste (`POWER_SAVE_TOGGLE`). Die Einstellung wird gespeichert und bleibt nach einem Neustart erhalten.

---

## WLAN und Web-Interface

### Verbindung herstellen

Die Uhr startet immer als WLAN-Access-Point:

| Parameter | Wert          |
|-----------|---------------|
| SSID      | `NixieClock`  |
| Passwort  | `nixie1234`   |
| IP-Adresse | `192.168.4.1` |

1. Mit dem WLAN `NixieClock` verbinden
2. Browser öffnen: `http://192.168.4.1`

### Heimnetz einrichten

Im Web-Interface unter **WLAN-Einrichtung**:

1. Auf **Scannen** klicken → verfügbare Netzwerke erscheinen
2. Gewünschtes Netzwerk auswählen
3. Passwort eingeben
4. **Verbinden** klicken → Uhr startet automatisch neu

Nach dem Neustart ist die Uhr sowohl über den eigenen Hotspot (`192.168.4.1`) als auch über das Heimnetz erreichbar. Die IP-Adresse im Heimnetz wird im Web-Interface angezeigt.

Zum Entfernen des Heimnetzes: SSID-Feld leer lassen und **Verbinden** klicken.

### NTP-Zeitsynchronisierung

Sobald die Uhr mit dem Heimnetz verbunden ist, synchronisiert sie die Zeit automatisch einmalig über NTP (`pool.ntp.org`). Die Zeitzone ist auf **Mitteleuropa (CET/CEST)** eingestellt. Der NTP-Status wird im Web-Interface angezeigt.

---

## Web-Interface – Funktionen

### Zeit stellen

- Stunden, Minuten und Sekunden manuell eingeben und **Übernehmen** klicken
- **Browser-Zeit** übernimmt automatisch die aktuelle Zeit des Geräts, von dem aus das Web-Interface geöffnet ist

### Helligkeit & Animation

| Einstellung           | Beschreibung                                      |
|-----------------------|---------------------------------------------------|
| Nixie-Helligkeit      | 4 Stufen (entspricht der LIGHT-Taste)             |
| NeoPixel-Helligkeit   | Schieberegler 10–255                              |
| Animation             | Animationsmodus wählen (siehe unten)              |
| NeoPixel-Farbe (Hue)  | Grundfarbe für statische Modi und Puls            |
| Power-Save            | Auto-Dimmen nach 120 s ein- oder ausschalten      |

### Animationsmodi

| Modus              | Beschreibung                                                        |
|--------------------|---------------------------------------------------------------------|
| Rainbow            | Farbverlauf wandert über die 6 Hintergrund-LEDs                     |
| Statisch Warmweiß  | Alle LEDs in warmweißem Orange                                      |
| Puls               | Sinusförmiges Auf- und Abdimmen der gesamten Beleuchtung            |
| Slot-Machine       | Schneller Farbwechsel während der Slot-Animation, sonst Rainbow     |

In allen Modi blinken die Doppelpunkt-LEDs sekundentaktgenau.

### Slot-Machine-Animation

Klick auf **Slot-Machine!** lässt alle 6 Röhren ihre Ziffer schnell durchrollen. Jede Röhre stoppt nacheinander (mit ca. 180 ms Versatz) bei der aktuellen Uhrzeit.

Im Modus *Slot-Machine* wird die Animation außerdem automatisch **alle 10 Sekunden** ausgelöst.

### Fernbedienung anlernen

In der Karte **IR-Fernbedienung** werden alle 7 Funktionen als Tabelle dargestellt. Neben jeder Funktion steht der aktuell gespeicherte Code (z. B. `0xFFA25D`) oder `—` wenn noch kein Code hinterlegt ist.

**Anlernen:**
1. Auf **Anlernen** neben der gewünschten Funktion klicken
2. Die Zeile blinkt orange und zeigt „Taste auf Fernbedienung drücken…"
3. Beliebige Taste auf der Fernbedienung drücken → Code wird sofort gespeichert
4. Nach **10 Sekunden** ohne Eingabe bricht der Anlernmodus automatisch ab

**Code löschen:** Auf **✕** neben der Funktion klicken.

Es können beliebige IR-Fernbedienungen verwendet werden (NEC, Samsung, Sony, RC5 u. v. m.). Jede Funktion kann unabhängig belegt werden.

---

## Einstellungen-Persistenz

Folgende Einstellungen werden dauerhaft im Flash-Speicher gespeichert und nach einem Neustart automatisch wiederhergestellt:

- Nixie-Helligkeit
- NeoPixel-Helligkeit
- Animationsmodus
- WLAN-Zugangsdaten
- IR-Codes (alle 7 Funktionen)
- Power-Save aktiviert/deaktiviert

---

## Technische Daten

| Eigenschaft         | Wert                          |
|---------------------|-------------------------------|
| Mikrocontroller     | ESP32-S3                          |
| Nixie-Röhren        | 6 Stück (Multiplexing)            |
| NeoPixel            | 10x WS2812B                       |
| Echtzeituhr         | DS1302                            |
| IR-Empfänger        | VS1838 (38 kHz), Pin IO48         |
| Versorgungsspannung | 5 V (Logik) + HV für Nixies       |
| Nixie-Hochspannung  | 170–180 V DC                      |
| Web-Interface       | Port 80, HTTP                     |
| NTP-Zeitzone        | CET-1CEST (Mitteleuropa)          |
