XXD=xxd
HEADERS=jquery.js.h jquery-ui.css.h jquery-ui.js.h licenses.txt.h OrbitControls.js.h\
	PLYLoader.js.h three.min.js.h showScan.js.h CanvasRenderer.js.h Projector.js.h SoftwareRenderer.js.h
PRAGMA=\#pragma once
ECHO=echo

all: $(HEADERS)

jquery.js.h: jquery.js
	$(ECHO) "$(PRAGMA)\n" > jquery.js.h
	$(XXD) -i jquery.js >> jquery.js.h

jquery-ui.css.h: jquery-ui.css
	$(ECHO) "$(PRAGMA)\n" > jquery-ui.css.h
	$(XXD) -i jquery-ui.css >> jquery-ui.css.h

jquery-ui.js.h: jquery-ui.js
	$(ECHO) "$(PRAGMA)\n" > jquery-ui.js.h
	$(XXD) -i jquery-ui.js >> jquery-ui.js.h

licenses.txt.h: licenses.txt
	$(ECHO) "$(PRAGMA)\n" > licenses.txt.h
	$(XXD) -i licenses.txt >> licenses.txt.h

OrbitControls.js.h: OrbitControls.js
	$(ECHO) "$(PRAGMA)\n" > OrbitControls.js.h
	$(XXD) -i OrbitControls.js >> OrbitControls.js.h
	
PLYLoader.js.h: PLYLoader.js
	$(ECHO) "$(PRAGMA)\n" > PLYLoader.js.h
	$(XXD) -i PLYLoader.js >> PLYLoader.js.h

three.min.js.h: three.min.js
	$(ECHO) "$(PRAGMA)\n" > three.min.js.h
	$(XXD) -i three.min.js >> three.min.js.h

showScan.js.h: showScan.js
	$(ECHO) "$(PRAGMA)\n" > showScan.js.h
	$(XXD) -i showScan.js >> showScan.js.h

CanvasRenderer.js.h: CanvasRenderer.js
	$(ECHO) "$(PRAGMA)\n" > CanvasRenderer.js.h
	$(XXD) -i CanvasRenderer.js >> CanvasRenderer.js.h

Projector.js.h: Projector.js
	$(ECHO) "$(PRAGMA)\n" > Projector.js.h
	$(XXD) -i Projector.js >> Projector.js.h

SoftwareRenderer.js.h: SoftwareRenderer.js
	$(ECHO) "$(PRAGMA)\n" > SoftwareRenderer.js.h
	$(XXD) -i SoftwareRenderer.js >> SoftwareRenderer.js.h

clean:
	rm *.js.h *.css.h *.txt.h