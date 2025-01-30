"use strict";

var xmlHttp = createXmlHttpObject();
var loadingdiv = null;        // le div d'attente pour l'upload
var get_time = true;
var request_busy = false;
var pending_request = null;

// web events
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

function ESP_Request(send_request, params = null) {
  // Le server n'est pas prêt, on met la requête en attente
  if (!serverIsReady(xmlHttp)) {
    pending_request = {"request": send_request, "params": params};
    return;
  }

  request_busy = true;
  if (send_request === "getFile")
  {
    xmlHttp.open("POST", "/getFile", true);
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

  if (send_request === "delFile")
  {
    xmlHttp.open("POST", "/delFile", true);
    xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlHttp.onreadystatechange = function() {
      if (this.status == 204) {
        document.getElementById("delFile").value = "";
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
    xmlHttp.send(params[0] + "&UART=1");  //  + "&UART=1" to send to UART
    request_busy = false;
    return;
  }
  else
    {
  // Opération par défaut : send_request is the param like PARAM=VALUE
  //    alert(send_request);
      xmlHttp.open("PUT","/operation",true);
      xmlHttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xmlHttp.onreadystatechange = null;
      xmlHttp.send(send_request);
      request_busy = false;
    }
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
/*
  // On demande l'heure une fois sur deux
  if (get_time)
  {
    xmlHttp.open("GET","/getTime",true);
    xmlHttp.onreadystatechange = handleServer_getTime;
    xmlHttp.send(null);
    get_time = false;
  }
  else
  // On demande les dernières données UART
  {
    xmlHttp.open("GET","/getUARTData",true);
    xmlHttp.onreadystatechange = handleServer_getUARTData;
    xmlHttp.send(null);
    get_time = true;
  }
*/
  // On demande toutes les dernières données
  xmlHttp.open("GET","/getLastData",true);
  xmlHttp.onreadystatechange = handleServerResponse;
  xmlHttp.send(null);

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
      if (values.length == 2)
        document.getElementById("data_uart").innerHTML = values[1];
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
  var unix = Math.round(date.getTime()/1000) - date.getTimezoneOffset() * 60; // UTC time soit GMT+00:00

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
  ESP_Request("getFile", ["/log.txt", "log.txt"]);
}

function deleteLOG() {
  if (window.confirm("Etes-vous sûr de vouloir supprimer le fichier log ?")) {
    ESP_Request("delFile", ["log.txt"]);
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

function toggleOled() {
  ESP_Request("Toggle_Oled=1");
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
  let CurrentPage = document.getElementById(pageName);
  CurrentPage.style.display = "block";
  elmnt.style.backgroundColor = color;
  elmnt.style.cursor = 'default';
}

window.addEventListener("load", (event) => {
  loadingdiv = document.getElementById("loading");

  // Get the element with id="defaultOpen" and click on it
  document.getElementById("defaultOpen").click();

  setTimeout('process()', 2000);
});

/*
// Autre solution à la place de addEventListener
window.onload = function() {
  loadingdiv = document.getElementById("loading");

  // Get the element with id="defaultOpen" and click on it
  document.getElementById("defaultOpen").click();

  setTimeout('process()', 2000);
}
*/