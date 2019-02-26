all:
	+$(MAKE) -C contrib
	+$(MAKE) -C src
	
clean:
	+$(MAKE) clean -C contrib
	+$(MAKE) clean -C src
	
startup: all
	+$(MAKE) startup -C src
	
dist: all
	+$(MAKE) dist -C src

run:
	+$(MAKE) run -C src

devrun:
	svn update
	+$(MAKE) stop
	+$(MAKE) run

stop:
	sudo killall freelss
