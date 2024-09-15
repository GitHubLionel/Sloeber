"use strict";

var xmlHttp = createXmlHttpObject();
var loadingdiv = null;        // le div d'attente pour l'upload
var get_time = true;
var request_busy = false;
var pending_request = null;
var CurrentPage = null;       // The current page
var first_init = true;        // Premier affichage de la page, on rafraichit les champs constants
var first_graphe = true;      // Initialisation du graphe

// Pour le Cirrus
var current_register = null;
var V1_SCALE = 242.0;
var I1_SCALE = 53.55;
var V2_SCALE = 242.0;
var I2_SCALE = 53.55;
var SCALE = [242.0, 53.55, 242.0, 53.55];
var get_scale = false;

function createXmlHttpObject() {
  if (window.XMLHttpRequest) {
    xmlHttp = new XMLHttpRequest();
  } else {
    xmlHttp = new ActiveXObject('Microsoft.XMLHTTP');
  }
  xmlHttp.onerror = onError;
  return xmlHttp;
}

function onError(event) {
  console.error("Une erreur " + event.target.status + " s'est produite au cours de la réception du document.");
  // On annule la requete et on poursuit
  if (event.target.status != 0)
  {
    request_busy = false;
    xmlHttp.onreadystatechange = null;
    process();
  }
}

// Test if the server can accept a new request
function serverIsReady(server) {
  return ((!request_busy) && (server.readyState == 0 || server.readyState == 4));
}

// Test if we have received the response
function requestIsOK(server) {
//  console.log('readyState: ', request.readyState);
//  console.log('status: ', request.status);
//  console.log('****');
  return (server.readyState == 4 && server.status == 200);
}

function ESP_Request(send_request, params) {
  // Le server n'est pas prêt, on met la requête en attente
  if (!serverIsReady(xmlHttp)) {
    pending_request = {"request": send_request, "params": params};
    return;
  }

  request_busy = true;
  if (send_request === "getfile")
  {
    xmlHttp.open("POST", "/getfile", true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = function() {
      if (requestIsOK(this)) {

        var blob = new Blob([this.responseText],
                    { type: "text/plain; charset=utf-8" });

        // Save the file with FileSaver.js
        saveAs(blob, params[1]);
      }
      wait_Upload(false);
      request_busy = false;
    }
    xmlHttp.send("FILE=" + params[0]);
    wait_Upload(true);
    return;
  }

  if (send_request === "delfile")
  {
    xmlHttp.open("POST", "/delfile", true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = function() {
      if (this.status == 204) {
        document.getElementById("delfile").value = "";
      }
  //          else alert("Not found");
      request_busy = false;
    }
    xmlHttp.send("FILE=" + params[0]);
    return;
  }

  if (send_request === "listfile")
  {
    xmlHttp.open("GET","/listfile?DIR=" + params[0],true);
    xmlHttp.onreadystatechange = function() {
      if (requestIsOK(this)) {
        var xmlResponse = xmlHttp.responseText;
        if (xmlResponse === null) return;
        document.getElementById("data_json").innerHTML = xmlResponse;
      }
      request_busy = false;
    }
    xmlHttp.send(null);
    return;
  }

  if (send_request === "resetESP")
  {
    xmlHttp.open("PUT","/resetESP",true);
    xmlHttp.onreadystatechange = null;
    xmlHttp.send(null);
    request_busy = false;
    return;
  }

  if (send_request === "setTime")
  {
    xmlHttp.open("PUT","/setTime",true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = null;
    xmlHttp.send(params[0]);  //  + "&UART=1" to send to UART
    request_busy = false;
    return;
  }

  if (send_request === "getCirrus")
  {
    xmlHttp.open("PUT","/getCirrus",true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = handleServer_getRegister;
    xmlHttp.send(params[0]);
    return;
  }

  if (send_request === "Update_CSV")
  {
    xmlHttp.open("POST", "/getfile", true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = function()
    {
      if (requestIsOK(this)) {
        let size = 4;
        // 4 pour data (time, P, U, T)
        // 3 pour energy (time, Ec, Es)
        if (params[1] === "data")
          size = 6;
        else size = 3;
        var data = parseCSV(this.responseText, size);
        updateGraph(data, params[1], false);
      }
      if (this.status == 204) {
        alert("Pas de fichier");
      }
      loadingdiv.style.visibility = "hidden";
      request_busy = false;
    }
    xmlHttp.send("FILE=" + params[0]);
    loadingdiv.style.visibility = "visible";
    return;
  }

  // Opération par défaut : send_request is the param
//  alert(send_request);
  xmlHttp.open("PUT","/operation",true);
  xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xmlHttp.onreadystatechange = null;
  xmlHttp.send(send_request);
  request_busy = false;
}

function process() {
  if (!serverIsReady(xmlHttp))
  {
    setTimeout('process()', 1000);
    return;
  }

  // On a une requête en attente
  if (pending_request != null)
  {
    ESP_Request(pending_request.request, pending_request.params);
    pending_request = null;
    setTimeout('process()', 1000);
    return;
  }

  // Les opérations de base qu'on doit répéter
  request_busy = true;

  // On demande toutes les dernières données
  if (first_init)
  {
    first_init = false;
    xmlHttp.open("GET","/initialisation",true);
    xmlHttp.onreadystatechange = handleServerInitialization;
    xmlHttp.send(null);
  }
  else 
    if (first_graphe)
    {
      first_graphe = false;
      request_busy = false;
      ESP_Request("Update_CSV", ["/data.csv", 'data']);
    }
    else
      {
        xmlHttp.open("GET","/getLastData",true);
        xmlHttp.onreadystatechange = handleServerResponse;
        xmlHttp.send(null);
      }
  setTimeout('process()', 1000);
}

function handleServer_getTime() {
  if (requestIsOK(xmlHttp))
  {
    var xmlResponse = xmlHttp.responseText;
    request_busy = false;

    if (xmlResponse === null) return;

    document.getElementById("time").innerHTML = xmlResponse;
  }
}

function handleServer_getUARTData() {
  if (requestIsOK(xmlHttp))
  {
    var xmlResponse = xmlHttp.responseText;
    request_busy = false;

    if (xmlResponse === null) return;

    document.getElementById("data_uart").innerHTML = xmlResponse;
  }
}
function handleServerInitialization() {
  if (requestIsOK(xmlHttp))
  {
    var xmlResponse = xmlHttp.responseText;
    request_busy = false;

    if (xmlResponse === null) return;

//    console.log(xmlResponse);

    // On suppose que les infos sont séparées par #
    // SSRType#P_CE#Percent#T_SSR#T_Relais#T_Clavier
    var values = xmlResponse.split('#');

    if (values.length >= 1)
    {
      document.getElementById("percent").checked = (values[0] === "true");
      document.getElementById("powerSSR").value = values[1]; 
      document.getElementById('Dimmer').value = values[2];
      document.getElementById('amount').value = parseInt(values[2]).toString() + "%";
      if (values[3] === "ON")
        document.getElementById("label_SSR").innerHTML = "SSR ON";
      else
        document.getElementById("label_SSR").innerHTML = "SSR OFF";
      if (values[4] === "ON")
        document.getElementById("label_Relay").innerHTML = "Relais ON";
      else
        document.getElementById("label_Relay").innerHTML = "Relais OFF";
      if (values[5] === "ON")
        document.getElementById("label_Keyboard").innerHTML = "Clavier ON";
      else
        document.getElementById("label_Keyboard").innerHTML = "Clavier OFF";
    }
  }
}

function handleServerResponse() {
  if (requestIsOK(xmlHttp))
  {
    var xmlResponse = xmlHttp.responseText;
    request_busy = false;

    if (xmlResponse === null) return;

//    console.log(xmlResponse);

    // On suppose que les infos sont séparées par #
    var values = xmlResponse.split('#');

    if (values.length >= 1)
    {
      document.getElementById("time").innerHTML = values[0];
      document.getElementById("_Conso_P").innerHTML = values[1] + " W";
      document.getElementById("_Urms").innerHTML = values[2] + " V";
      document.getElementById("_Conso_E").innerHTML = values[3] + " Wh";
      document.getElementById("_Conso_S").innerHTML = values[4] + " Wh";
      document.getElementById("_cosphi").innerHTML = values[5];
      document.getElementById("_T_CS").innerHTML = values[6] + "°C";
      document.getElementById("_T_DS_int").innerHTML = values[7] + "°C";
      document.getElementById("_T_DS_ext").innerHTML = values[8] + "°C";

      // Extra data for graphe : puissance, tension, températures
      if (values.length > 9)
      {
        let theUnixtime = Math.round((new Date()).getTime()/1000);
        let data = [theUnixtime];
        for (let i=9; i<values.length; i++) {
          data.push(values[i]);
        }
        // On rajoute les deux températures
        data.push(values[7]);
        data.push(values[8]);
        updateGraph(data, 'data', true);
      }
    }
  }
}

function wait_Upload(visible) {
  if (visible === true)
  loadingdiv.style.visibility = "visible";
  else
    loadingdiv.style.visibility = "hidden";
  return true;
}

// ***************************
// Time function
// ***************************
function createDateTime() {
  var date = new Date();

  var day = date.getDate();
  var month = date.getMonth() + 1;
  var year = date.getFullYear()-2000;  //   getYear
  var hours = date.getHours();
  var minutes = date.getMinutes();
  var seconds = date.getSeconds();
  var unix = Math.round(date.getTime()/1000); // UTC time soit GMT+00:00

  if (day < 10) day = "0" + day;
  if (month < 10) month = "0" + month;
  if (hours < 10) hours = "0" + hours;
  if (minutes < 10) minutes = "0" + minutes;
  if (seconds < 10) seconds = "0" + seconds;

  var today = "TIME=" + [day, month, year].join('-') + "H" + [hours, minutes, seconds].join('-')+ "U" + unix;

//  alert(today);

  return today;
}

function sendDateTime() {
  if (window.confirm("Etes-vous sûr de vouloir changer l'heure ?")) {
    ESP_Request("setTime", [createDateTime()]);
  }
}

// ***************************
// Get the data from ESP
// ***************************

function loadLOG() {
  ESP_Request("getfile", ["/log.txt", "log.txt"]);
}

function deleteLOG() {
  if (window.confirm("Etes-vous sûr de vouloir supprimer le fichier log ?")) {
    ESP_Request("delfile", ["log.txt"]);
  }
}

// ***************************
// Divers
// ***************************

function listDir() {
  ESP_Request("listfile", ["/"]);
}

function resetESP() {
  if (window.confirm("Etes-vous sûr de vouloir faire un RESET ?")) {
    ESP_Request("resetESP");
  }
}

function doToggle(obj) {
  if (obj === "Oled")
  {
    ESP_Request("Toggle_Oled");
    return;
  }

  if (obj === "SSRType")
  {
    let percent = (document.getElementById("percent").checked);
    ESP_Request("SSRType=" + percent);
    // Le changement de mode éteint le SSR
    let label = document.getElementById("label_SSR");
    label.innerHTML = "SSR OFF";
    return;
  }

  if (obj === "SSR")
  {
    ESP_Request("Toggle_SSR");
    let label = document.getElementById("label_SSR");
    if (label.innerHTML === "SSR ON")
      label.innerHTML = "SSR OFF";
    else label.innerHTML = "SSR ON";
    return;
  }

  if (obj === "Relay")
  {
    ESP_Request("Toggle_Relay");
    let label = document.getElementById("label_Relay");
    if (label.innerHTML === "Relais ON")
      label.innerHTML = "Relais OFF";
    else label.innerHTML = "Relais ON";
    return;
  }

  if (obj === "Keyboard")
  {
    ESP_Request("Toggle_Keyboard");
    let label = document.getElementById("label_Keyboard");
    if (label.innerHTML === "Clavier ON")
      label.innerHTML = "Clavier OFF";
    else label.innerHTML = "Clavier ON";
    return;
  }
}

// ***************************
// Dimmer SSR
// ***************************
function sendCEPower() {
  let power = document.getElementById("powerSSR").value;
  ESP_Request("CEPower=" + power);
}

function setDimme() {
  if (document.getElementById("percent").checked)
  {
    let dimmer = document.getElementById('Dimmer');
    ESP_Request("Dimmer=" + dimmer.value);
  }
  else
    alert("Seulement avec pourcentage.");
}

// ***************************
// Window onload event
// ***************************
function openPage(pageName, elmnt, color) {
  var i, tabcontent, tablinks;
  // Hide all the pages
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }
  // remove tab color
  tablinks = document.getElementsByClassName("tablink");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].style.backgroundColor = "";
    tablinks[i].style.cursor = 'pointer';
  }
  // Display the new page
  CurrentPage = document.getElementById(pageName);
  CurrentPage.style.display = "block";
  elmnt.style.backgroundColor = color;
  elmnt.style.cursor = 'default';
}

window.onload = function() {
  loadingdiv = document.getElementById("loading");

  // Initialise le graphe
  initializeGraphe();

  // Get the element with id="defaultOpen" and click on it
  document.getElementById("defaultOpen").click();

  setTimeout('process()', 2000);
}
