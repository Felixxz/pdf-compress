APP=pdf-compress
#CFLAGS=-O3 -m64
CFLAGS=-O0 -g3
all:$(APP)
$(APP): $(APP).c
	gcc -o$@ $(APP).c $(CFLAGS) `pkg-config gtk+-3.0 --cflags --libs`

clean:
	rm -f $(APP)

install: all
	install $(APP) /usr/bin -o root -g root -m 555
	for i in 16 24 32 48 64 96 128 256; \
	do \
		mkdir -p /usr/share/icons/hicolor/$(i)x$(i)/apps/; \
		install data/icons/$$i.png /usr/share/icons/hicolor/$${i}x$${i}/apps/$(APP).png -m 644; \
	done
