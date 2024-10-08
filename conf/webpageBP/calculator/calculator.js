// Taschenrechner Javascript

// globale Variablen:
var calculationFinished = false;
var kommaUsed = false;

/*TO DOs:
- Klammerncheck

Erledigt:
- keine doppelten Rechenzeichen
- löschen des Ergebnis bei erneuter Eingabe
- kommaCheck
- start ohne / * .
- Weiterrechnen (Zwischenergebnis)
*/


// Prüft, ob Komma bereits in Verwendung bzw. ob schon eine Zahl vor dem Komma getippt wurde.
function kommaCheck(operation) {
    var container = document.getElementById('result-area')
    // Komma nur, wenn nicht bereits in Verwendung, nicht am Beginn, nicht nach einem Rechenzeichen, und Rechnung noch nicht beendet
    if (kommaUsed === false && container.innerHTML !== '' && !container.innerHTML.endsWith(' ') && !calculationFinished ) {
        container.innerHTML += operation;
        // setzen der globalen Variable, damit Komma nicht nocheinmal verwendet werden kann
        kommaUsed = true;
    }
}

function appendOperation(operation, mathOperator) {
    var container = document.getElementById('result-area');

    // Überprüfung, ob Rechnung schon beendet wurde, falls ja - Rücksetzen der Variablen
    if (calculationFinished && mathOperator === false) {
        // Löschen des Eingabefeldes
        deleteAll();
        //Rücksetzen der globalen Flags für beendete Rechnung und verwendetes Komma
        calculationFinished = false;
        kommaUsed = false;
    // Option zum Weiterrechnen bei Drücken auf Rechenzeichen
    } else if (calculationFinished && mathOperator) {
        calculationFinished = false;
    }
    // Tausch des Rechenzeichens bei doppeltem Rechenzeichen
    if (container.innerHTML.endsWith(' ') && mathOperator) {
        // schneidet 3 Zeichen weg (2 Leerzeichen + Operator)
        container.innerHTML = container.innerHTML.slice(0,-3);
        container.innerHTML += operation;
    // Fehlermeldung, falls bei leerem Eingabefeld ein Rechenzeichen gedrückt wird
    } else if (container.innerHTML === '' && mathOperator && operation !== ' - ' && operation !== ' + '  ) {
        container.innerHTML = 'error';
        calculationFinished = true;
    }
    // Hinzufügen der Zahl bzw. des Rechenzeichen
    else {
        document.getElementById('result-area').innerHTML +=operation;
        if (operation.endsWith (' ')) {kommaUsed = false}
    }



};
// Ergebnis ermitteln
function calculateResult() {
    var container = document.getElementById('result-area');
    var result = eval(container.innerHTML);
    container.innerHTML = result;
    calculationFinished = true;
};

// Eingabefeld leeren
function deleteAll(){
    document.getElementById('result-area').innerHTML = '';
}

// letztes Zeichen löschen
function deleteLast(){
    var container = document.getElementById('result-area');
    if (calculationFinished === false) {
        if (container.innerHTML.endsWith(' ')) {
            container.innerHTML = container.innerHTML.slice(0,-3);
        } else {
            container.innerHTML = container.innerHTML.slice(0,-1);
        }
    }
}
