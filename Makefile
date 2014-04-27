barcodedriver: barcodedriver.o
	gcc -o barcodedriver barcodedriver.o -lusb -lX11 -lXtst

barcodedriver.o : barcodedriver.c
	gcc -c -o barcodedriver.o barcodedriver.c

clean:
	rm -f *.o

mrproper: clean
	rm barcodedriver
