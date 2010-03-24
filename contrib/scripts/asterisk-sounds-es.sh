#!/usr/bin/env bash

#####################################################################
# Script de instalacion y configuracion de las voces de VoIPNovatos #
# Version 0.3                                                       #
# Por Elio Rojano  (http://www.sinologic.net)                       #
# Modificado por Saúl Ibarra (http://www.saghul.net)                #
# Modificado por Jon Bonilla (http://sipdoc.net)		    #
# Licencia: GPL                                                     #
#####################################################################


ESPANOL=`echo $LANG |grep "es"`
if [ "$ESPANOL" ]; then
    MSG1="Aceptas la licencia de uso? [S/N]: "
    MSG2="Escribe tu nombre: "
    MSG3="Escribe tu email: "
    MSG4="Escribe la empresa que tendra esta instalación: "
    MSG5="Registrando usuario ..."
    MSG6="Descargando el conjunto de sonidos seleccionado..."
    MSG8="Creando enlaces para compatibilizar Asterisk..."
    MSG12="Instalando ..."
    MSG14="¿Quieres los sonidos en GSM? [S/N]: "
    MSG15="¿Quieres los sonidos en ALAW? [S/N]: "
    MSG16="¿Quieres los sonidos en ULAW? [S/N]: "
    MSG17="¿Quieres los sonidos en G729? [S/N]: "
else
    MSG1="Are you AGREE with the license? [Y/N]: "
    MSG2="Type your name: "
    MSG3="Type your email: "
    MSG4="Type your company: "
    MSG5="Registry user ..."
    MSG6="Downloading sound set CORE ..."
    MSG7="Downloading sound set EXTRA ..."
    MSG8="Creating symbolic links to add some more compatibility ..."
    MSG12="Installing ..."
    MSG14="Do you wish to install GSM format sounds? [Y/N]: "
    MSG15="Do you wish to install ALAW format sounds? [Y/N]: "
    MSG16="Do you wish to install ULAW format sounds? [Y/N]: "
    MSG17="Do you wish to install G729 format sounds? [Y/N]: "
fi

if [ -d /var/lib/asterisk/sounds ]; then
    pushd /var/lib/asterisk/sounds
    mkdir -p es
    wget -qc http://www.voipnovatos.es/voces/licenciadeuso.txt -O- | iconv -f latin1  -t utf8 | more
    echo ""
    while [ ! "$ACEPTADA" ]; do
	echo -n $MSG1 && read ACEPTADA
	case $ACEPTADA in
    	    S|s|Y|y) ACEPTADA="Si" ;;
    	    N|n) ACEPTADA="No" ;;
    	    *) ACEPTADA="" ;;
	esac
    done
    if [ "$ACEPTADA" == "Si" ]; then    
	while [ ! "$NOMBRE" ]; do echo -n $MSG2; read NOMBRE; done
	while [ ! "$EMAIL" ]; do echo -n $MSG3; read EMAIL; done
	while [ ! "$EMPRESA" ]; do echo -n $MSG4; read EMPRESA; done


        # Qué formatos quieres?
        echo ""
        while [ ! "$format_gsm" ]; do
    	echo -n $MSG14 && read format_gsm
    	case $format_gsm in
        	    S|s|Y|y) format_gsm="Si" ;;
        	    N|n) format_gsm="No" ;;
        	    *) format_gsm="" ;;
    	esac
        done

        echo ""
        while [ ! "$format_alaw" ]; do
    	echo -n $MSG15 && read format_alaw
    	case $format_alaw in
        	    S|s|Y|y) format_alaw="Si" ;;
        	    N|n) format_alaw="No" ;;
        	    *) format_alaw="" ;;
    	esac
        done

        echo ""
        while [ ! "$format_ulaw" ]; do
    	echo -n $MSG16 && read format_ulaw
    	case $format_ulaw in
        	    S|s|Y|y) format_ulaw="Si" ;;
        	    N|n) format_ulaw="No" ;;
        	    *) format_ulaw="" ;;
    	esac
        done

        echo ""
        while [ ! "$format_g729" ]; do
    	echo -n $MSG17 && read format_g729
    	case $format_g729 in
        	    S|s|Y|y) format_g729="Si" ;;
        	    N|n) format_g729="No" ;;
        	    *) format_g729="" ;;
    	esac
        done


	# Para hacer uso de las locuciones de cara al publico, deben enviarse estos datos al creador...
	echo $MSG5
	wget -cqF "http://voipnovatos.es/voces.php?name=$NOMBRE&email=$EMAIL&empresa=$EMPRESA&format=$formato" -O /dev/null
	sleep 1
        echo $MSG12
        sleep 1
        echo $MSG6
        if [ "$format_gsm" == "Si" ]; then    
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-core-sounds-es-gsm-1.4.tar.gz 
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-extra-sounds-es-gsm-1.4.tar.gz
        fi
        if [ "$format_alaw" == "Si" ]; then    
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-core-sounds-es-alaw-1.4.tar.gz 
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-extra-sounds-es-alaw-1.4.tar.gz
        fi
        if [ "$format_ulaw" == "Si" ]; then    
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-core-sounds-es-ulaw-1.4.tar.gz 
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-extra-sounds-es-ulaw-1.4.tar.gz
        fi
        if [ "$format_g729" == "Si" ]; then    
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-core-sounds-es-g729-1.4.tar.gz 
            wget -c  http://www.voipnovatos.es/voces/voipnovatos-extra-sounds-es-g729-1.4.tar.gz
        fi

        for f in *.tar.gz; do tar xvzf $f; done
        rm -f *.tar.gz

	#Arreglando permisos...
	chmod  644 /var/lib/asterisk/sounds/dictate/es/*
	chmod  644 /var/lib/asterisk/sounds/digits/es/*
	chmod  644 /var/lib/asterisk/sounds/followme/es/*
	chmod  644 /var/lib/asterisk/sounds/letters/es/*
	chmod  644 /var/lib/asterisk/sounds/phonetic/es/*
	chmod  644 /var/lib/asterisk/sounds/silence/es/*
	chmod  755 /var/lib/asterisk/sounds/dictate
	chmod  755 /var/lib/asterisk/sounds/digits
	chmod  755 /var/lib/asterisk/sounds/followme
	chmod  755 /var/lib/asterisk/sounds/letters
	chmod  755 /var/lib/asterisk/sounds/phonetic
	chmod  755 /var/lib/asterisk/sounds/silence
	chmod  755 /var/lib/asterisk/sounds/dictate/es
	chmod  755 /var/lib/asterisk/sounds/digits/es
	chmod  755 /var/lib/asterisk/sounds/followme/es
	chmod  755 /var/lib/asterisk/sounds/letters/es
	chmod  755 /var/lib/asterisk/sounds/phonetic/es
	chmod  755 /var/lib/asterisk/sounds/silence/es
	chmod  755 /var/lib/asterisk/sounds/es
	chmod  644 /var/lib/asterisk/sounds/es/*
        
	echo $MSG8
        ln -s /var/lib/asterisk/sounds/dictate/es /var/lib/asterisk/sounds/es/dictate >/dev/null 2>/dev/null
        ln -s /var/lib/asterisk/sounds/digits/es /var/lib/asterisk/sounds/es/digits >/dev/null 2>/dev/null
        ln -s /var/lib/asterisk/sounds/followme/es /var/lib/asterisk/sounds/es/followme >/dev/null 2>/dev/null
        ln -s /var/lib/asterisk/sounds/letters/es /var/lib/asterisk/sounds/es/letters >/dev/null 2>/dev/null
        ln -s /var/lib/asterisk/sounds/phonetic/es /var/lib/asterisk/sounds/es/phonetic >/dev/null 2>/dev/null
        ln -s /var/lib/asterisk/sounds/silence/es /var/lib/asterisk/sounds/es/silence >/dev/null 2>/dev/null
      
    fi
fi

popd >/dev/null
exit 0

