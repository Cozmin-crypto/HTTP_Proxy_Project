Proiect PSO: HTTP Proxy în C
Scopul proiectului
Proiectul meu este un proxy HTTP făcut în C, care rulează pe Linux într-o mașină virtuală. Practic stă între browser și internet: ia cererea de la browser, mă lasă să mă uit la ea și să o modific, apoi o trimite mai departe la server. La fel se întâmplă și cu răspunsul care vine înapoi.
Trebuie să pot intercepta traficul HTTP, să editez headerele și conținutul și să am reguli de modificare a conținutului (filtre și schimbarea link-urilor).
Ce trebuie să facă aplicația
Cerintele:
Să asculte pe un port (de exemplu 8080), iar browserul să fie setat să îl folosească ca proxy.
Să primească cererile HTTP (GET, POST, PUT) și să le trimită mai departe către serverul potrivit.
Să poată opri o cerere sau un răspuns, ca să le pot vedea și modifica (headere și conținut) înainte să plece mai departe.
Să poată lucra cu mai mulți clienți în același timp.
Să aibă reguli automate de modificare, scrise într-un fișier: filtre de conținut (blocare sau înlocuire după domeniu, cuvinte cheie sau tip de conținut) și schimbarea link-urilor din paginile HTML.
Să țină un istoric al traficului, ca să îl pot consulta.

Cum funcționează
Browserul trimite cererile către proxy, iar proxy-ul le trimite mai departe către serverul real și întoarce răspunsul. Pe drum, mesajele trec prin câteva părți:
ascultătorul, care primește conexiunile de la browser
parserul HTTP, care citește linia de start, headerele și conținutul
interceptorul, unde mesajul poate fi oprit și modificat
motorul de reguli, care aplică filtrele și schimbarea link-urilor
partea care se conectează la server și trimite cererea
istoricul, unde se salvează tot traficul
Pentru fiecare client nou pornim un thread, iar listele folosite de mai multe threaduri, le protejam cu mutex.
Pași de lucru
P1: cerințele și această documentație
P2: proiectarea și un prototip mic, care doar trimite mai departe cererile GET
P3: evaluare intermediară
P4: parser HTTP complet și mai mulți clienți în paralel
P5: evaluare intermediară 2, cam jumătate din proiect gata
P6: interceptare și editare manuală, reguli de conținut, schimbarea link-urilor
P7: evaluare finală, cu demonstrație
Testare
Testarea se face în VM, pe site-uri HTTP
Folosesc curl -x http://127.0.0.1:8080 http://example.com și compar rezultatul cu cel fără proxy.
Setez proxy-ul în Firefox și deschid site-uri HTTP simple.
Încerc cereri greșite, un server care nu există și mai multe cereri curl în paralel.

