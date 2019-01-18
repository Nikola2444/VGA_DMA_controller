# VGA_DMA_controller

VGA_DMA_Project *Sadrzi Vivado projekat za koji se piše drajver. Kako bi se projekat automatski napravio potrebno je pokrenuti
Master.tcl skriptu. To se radi tako sto se prvo pokrene Vivado alat  a zatim se klikne na Tools -> Run Tcl Script. Potom se pronadje fajl Master.tcl,
selektuje se i klikne Open. Tada kreće proces automatskog generisanja, sintetizovanja i implementacije sistema. Ovaj proces može da potraje 10 minuta.

BOOT_files folder sadrži unutar sebe fajlove koje je potrebno kopirati na BOOT particiju microSD kartice, kako bi operativni mogao da vidi sistem za koji
se piše drajver.

SDK_files folder sadrži unutar sebe c kodove sa kojima može da se testira rad sistema u VIVADO SDK alatu. Da bi se to uradilo, neophodno nakon pokretanja tcl skripte pokrenuti  SDK
i u njemu napraviti novi aplikaciju sledećom komandom file->new->application. Izabrati hello world projekat, i tu kopirati date C fajlove.

Driver folder sadrži unutar sebe c kod koji predstavlja drajver preko koga se upravlja datim sistemom. Takodje unutar ovog foldera sa nalazi i make fajl koji se koristi
za kompajliranje drajvera.

Application folder sadrži unutar sebe test aplikaciju koja je zadužena da testira rad drajvera, odnosno da pošalje drajveru sliku koja treba da se pojavi na monitoru.

