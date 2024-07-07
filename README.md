# Utilisation de Sloeber avec l'ESP8266 et l'ESP32
Sloeber est un IDE basé sur <a href="https://www.eclipse.org/" target="_blank">Eclipse</a><br>
L'avantage de Sloeber est qu'il est fourni "plug and play". Y a juste à télécharger le fichier et à le dézipper. Dans le dossier "Doc", je détaille mon environnement de travail (voir Sloeber_Install.html). <br>

Le dossier "Library" contient les librairies que j'utilise avec Sloeber. Voir Sloeber_Install.html pour une liste de directives associées.<br>

Dans le dossier "workspace", il y a les deux projets template qui me servent de base pour tout nouveau projet. En effet, un programme Arduino pour l'ESP à toujours la même structure : ouverture UART, oled, création d'une connexion et page web de base.<br>

Le dossier "Divers" contient un programme de type Terminal qui ouvre une console (comme par exemple MiniTerm, RealTerm). Cela permet d'afficher le log et d'envoyer certaines instructions à l'ESP. Il a en plus des fonctions spécifiques pour communiquer avec le Cirrus.
