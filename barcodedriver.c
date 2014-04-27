#include <stdio.h>
#include <usb.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <X11/extensions/XTest.h>
#define XK_LATIN1 // pour les lettres de l'alphabet
#define XK_MISCELLANY // pour shift et return
#include <X11/keysymdef.h>
#include <stdlib.h>

#define DUREE_PRESSION 2
#define MAXTRANSLATION 37


//sudo apt-get install libxtst-dev libusb-dev

struct _touche
{
    unsigned long int code;
    KeySym lettre;
};

struct _touche touche[MAXTRANSLATION] =
            // poids faible poids fort, caractere
            // par exemple 12 34 56 78 s'écrit 78 56 34 12
{
    {0x001E0000, XK_1},
    {0x001F0000, XK_2},
    {0x00200000, XK_3},
    {0x00210000, XK_4},
    {0x00220000, XK_5},
    {0x00230000, XK_6},
    {0x00240000, XK_7},
    {0x00250000, XK_8},
    {0x00260000, XK_9},
    {0x00270000, XK_0},
    {0x00280000, XK_Return},

    {0x00040002, XK_Q},
    {0x00050002, XK_B},
    {0x00060002, XK_C},
    {0x00070002, XK_D},
    {0x00080002, XK_E},
    {0x00090002, XK_F},
    {0x000A0002, XK_G},
    {0x000B0002, XK_H},
    {0x000C0002, XK_I},
    {0x000D0002, XK_J},
    {0x000E0002, XK_K},
    {0x000F0002, XK_L},
    {0x00100002, XK_colon},
    {0x00110002, XK_N},
    {0x00120002, XK_O},
    {0x00130002, XK_P},
    {0x00140002, XK_A},
    {0x00150002, XK_R},
    {0x00160002, XK_S},
    {0x00170002, XK_T},
    {0x00180002, XK_U},
    {0x00190002, XK_V},
    {0x001A0002, XK_Z},
    {0x001B0002, XK_X},
    {0x001C0002, XK_Y},
    {0x001D0002, XK_W}
};

KeySym translate(unsigned long int valeur)   // Traduction des codes du device en touche clavier (FR)
{
    int j;
    if (valeur != 0)
    {
        for (j = 0; j < MAXTRANSLATION; j++)
        {
            if (touche[j].code == valeur)
            {
                return touche[j].lettre;
            }
        }
    }
    return 0;
}

void driver_init(void)
{
    usb_init();                    //Initialisation de la librairie (par example determine le chemin du repertoire des bus et peripheriques)
    usb_find_busses();             // Enumère tous les bus USB du systemes
    usb_find_devices();            // Enumère tous les peripheriques sur tous les Bus présents)
}

struct usb_device *usb_find_My_device(int idV, int idP)    // recherche du device avec l'id Vendor et product
{
    struct usb_bus *busses;
    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_device *fdev;
    for (bus = usb_busses; bus; bus = bus->next)   // Parcours tous les bus USB
    {
        for (dev = bus->devices; dev; dev = dev->next)     // Pour chaque bus, parcours tous les appareils branchés
        {
            if ((dev->descriptor.idVendor == idV) && (dev->descriptor.idProduct == idP ))  // on vérifie si les coordonnées (id vendeur; id produit) de notre appareil corespond
            {
                return (dev);
            }
        }
    }
    return (0);
}

void unlinkKernelDriver(usb_dev_handle *hdl)   // fonction pour détacher le driver du kernel (usbhid)
{
    char str[100];
    memset(&str[0], 0, 100); // réinitialisation de la variable
    usb_get_driver_np(hdl, 0, &str[0], 100); // récupération du driver lié à l'appareil
    if (strlen(str) > 0)
    {
        usb_detach_kernel_driver_np(hdl, 0);
    }
}


int readDevice(usb_dev_handle *hdl, int timeOut)   // lecture des codes barre
{
    char donnees[16];
    int i;
    KeySym lettre;
    unsigned long int *myTouche;
    int longueur;
    Display *pDisplay;
    memset(&donnees[0], 0, sizeof(donnees)); // on remet les variables à 0
    myTouche = (unsigned long int *)&donnees[0];

    usb_resetep(hdl, 0x081);
    while (1)
    {
        longueur = usb_interrupt_read(hdl, 0x081, &donnees[0], sizeof(donnees), timeOut);
        if (longueur >= 0)  // il y avait quelque chose à lire ...
        {
            for (i = 0; i < sizeof(donnees) / 4; i++)
            {
                lettre = translate(myTouche[i]);
                if (lettre != 0)
                {
                    pDisplay = XOpenDisplay( NULL );
                    if (lettre != XK_Return)
                    {
                        XTestFakeKeyEvent ( pDisplay, XKeysymToKeycode( pDisplay, XK_Shift_L), True, DUREE_PRESSION ); // on presse shift
                    }
                    XTestFakeKeyEvent ( pDisplay, XKeysymToKeycode( pDisplay, lettre ), True, DUREE_PRESSION ); // on presse la touche correspondant à la lettre
                    XTestFakeKeyEvent ( pDisplay, XKeysymToKeycode( pDisplay, lettre ), False, DUREE_PRESSION ); // on relache la touche
                    if (lettre != XK_Return)
                    {
                        XTestFakeKeyEvent ( pDisplay, XKeysymToKeycode( pDisplay, XK_Shift_L), False, DUREE_PRESSION ); // on relache shift
                    }
                    XCloseDisplay(pDisplay);
                }
            }
        }
    }

    return 0;
}


int main (int argc, char *argv[])
{
    char str[100];
    char usage[] = "Usage :\n   -i xxxx:xxxx - Vendor:Product id\n   -t xxxx - usb timeout in ms (default = 0, waiting data)\n   -h - This help\n";

    if (getuid() > 0)
    {
        printf("Warning ! you must launch this programme as root !\n\n%s", usage);
        return 0;
    }

    // *************  Lecture des options *************

    char *liste_options = "i:t:h";
    char *strId;
    int option;
    int i;
    int e_opt = 0;
    int s_opt = 0;
    int r_opt = 0;
    int k_opt = 0;
    int idV = 0;
    int idP = 0;
    int timeout = 0;
    struct usb_device *barCodeReader;
    usb_dev_handle *barCodeReaderHandle;
    opterr = 0; // Pas de message d'erreur automatique

    fprintf(stderr, "Driver to get data from USB bar code reader\n07/2008 - C. MEICHEL\n\n");

    while ((option = getopt(argc, argv, liste_options)) != -1)
    {
        switch (option)
        {
        case 't' :
            timeout = atoi(optarg);
            break;
        case 'i' :
            strId = strdup(optarg);
            for (i = 0; (strId[i]) && (strId[i] != ':'); i++);
            if (strId[i] == 0)
            {
                printf("%s", usage);
                return 0;
            }
            strId[i] = 0;
            sscanf(strId, "%x", &idV);
            sscanf(&strId[i + 1], "%x", &idP);
            fprintf(stderr, "Listening %04X:%04X\n", idV, idP);
            break;
        case 'h' :
            printf("%s", usage);
            return 0;
            break;
        case '?' :
            printf("%s", usage);
            return 0;
            break;
        }
    }


    // *************  Initialise les devices *************
    driver_init();

    if ((idV) || (idP))
    {
        // *************  Recherche du device *************
        barCodeReader = usb_find_My_device(idV, idP);  // on recherche l'appareil "lecteur code barre" 0x04D9, 0x1400
        if (barCodeReader == 0)   // si on ne l'a pas trouvé ...
        {
            printf("       ! No Bar Code Reader found !\n");
            return 0;
        }

        barCodeReaderHandle = usb_open(barCodeReader);  // ouverture du device
        if (!barCodeReaderHandle)
        {
            return 0;    // si l'ouverture s'est mal passée ...
        }
        unlinkKernelDriver(barCodeReaderHandle); // on détache le driver du Kernel (usbhid)
        usb_reset(barCodeReaderHandle);

        readDevice(barCodeReaderHandle, timeout);

        usb_reset(barCodeReaderHandle);
        usb_close(barCodeReaderHandle);
    }
    else
    {
        printf("Bad Vendor or/and Product id !\n");
    }
    return 0;
}
