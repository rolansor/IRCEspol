/*
SERVIDOR IRC EN C++
Sistemas Operativos
ESPOL 2015-2016
Archivo canales.cpp
Grupo: Andres Sornoza, Fausto Mora, Wilson Enriquez
Adaptacion Simplificada de Michalis Zervos - http://michal.is
*/
 
#include "usuarios.h"

/* Clase Usuario */




Usuario::Usuario(int sockdscr, char * nombre_host, pthread_t threadid){
	if ( (buf =(char *) malloc(TAM_BUFFER)) < 0){
		return;
    }

	this->sock = sockdscr;
	strcpy(this->nombre_host, nombre_host);
	this->tid = threadid;

	this->buf[0] = this->nickname[0] = this->nombre_usuario[0] = this->nombre_real[0] = '\0';
	this->estado = NONE;
	this->num_canales = 0;
	pthread_mutex_init(&this->lock, NULL);


}


Usuario::~Usuario(){
	pthread_mutex_unlock(&this->lock);
	pthread_mutex_destroy(&this->lock);
	free(buf);
}

//Método Get para obtener el nickname del usuario.
int Usuario::getNickname(char *nick){
	if (nick == NULL){
		return -1;
    }
	strcpy(nick, this->nickname);
	return 0;
}

//Método Set para definir un nickname de usuario.
int Usuario::setNickname(char *nick){
	strcpy(this->nickname, nick);
}

//Método para obtener el Nombre del usuario.
int Usuario::getNombreUsuario(char *nombre_usu){
	if (nombre_usu == NULL){
		return -1;
    }
	strcpy(nombre_usu, this->nombre_usuario);
	return 0;
}

//Método Set para definir el nombre del usuario
int Usuario::setNombreUsuario(char *nombre_usu){
	strcpy(this->nombre_usuario, nombre_usu);
}


int Usuario::getNombreReal(char *nombre_r){
	if (nombre_r == NULL){
		return -1;
    }
	strcpy(nombre_r, this->nombre_real);
	return 0;
}


//Método Set para definir el nombre real del Usuario
int Usuario::setNombreReal(char *nombre_r){
	strcpy(this->nombre_real, nombre_r);
}

//Método Get para obtener el nombre del Host
int Usuario::getNombreHost(char *nombre_h){
	if (nombre_h == NULL){
		return -1;
    }
	strcpy(nombre_h, this->nombre_host);
	return 0;
}

//Función para obtener el número actual de canales
int Usuario::numCanales(){
	return this->num_canales;
}

//Método get para obtener el estado del usuario
Regstatus Usuario::getEstado(){
	return this->estado;
}

//Método Set para obtener el estado del Usuario.
void Usuario::setEstado(Regstatus rs){
	this->estado = rs;
}


int Usuario::getCanales(int chns[], int tam){

	if (tam < this->numCanales()){
		return -1;
    }

	for (int i = 0; i < tam; i++){
		chns[i] = this->canales_usuario[i];
    }

	return this->numCanales();
}


int Usuario::getCanales(int i){
	if (i < this->numCanales()){
		return this->canales_usuario[i];
    }else{
		return -1;
    }
}
//Retorna el numero del socket de la clase usuario
int Usuario::getSock(){
	return this->sock;
}

//Retorna el ID del thread de la clase usuario
pthread_t Usuario::getThreadId(){
	return this->tid;
}


int Usuario::join(char *chan){
	return this->join(obtenerCanalxNombre(chan));
}


int Usuario::join(int id){
	if (id < 0){
		return -1;
    }
	if (this->numCanales() >= MAX_CANALES_X_USUARIO){
		return -2;
    }
	if (this->isIn(id)){
		return -3;
    }
    
	this->canales_usuario[this->num_canales++] = id;
	canales[id]->anadirUsuario(this->nickname);
}


int Usuario::part(char *chan){
	return this->part(obtenerCanalxNombre(chan));
}


int Usuario::part(int id){
	int canal_pos;

	if (id < 0){
		return -1;
    }
	if (this->numCanales() <= 0){
		return -2;
    }
	canal_pos = this->buscarCanal(id);
	if (canal_pos < 0){
		return -3;
    }
	for (int i = canal_pos; i < this->numCanales()-1; i++){
		this->canales_usuario[i] = this->canales_usuario[i+1];
    }
	canales[id]->removerUsuario(this->nickname);

	return canales[id]->contarUsuarios();

}


int Usuario::buscarCanal(int id){
	for (int i = 0; i < this->numCanales(); i++){
		if ( this->canales_usuario[i] == id ){
			return i;
        }
    }
	return -1;
}


int Usuario::isIn(int id){
	return (this->buscarCanal(id) >= 0);
}


int Usuario::enviarMensaje(char * msg = NULL){
	int b_wr;

	if ( msg == NULL ){
		b_wr = write(this->sock, this->buf, strlen(this->buf));
		if (b_wr != strlen(this->buf)){
			return -1;
        }
        
	}else{	
		if (strlen(msg) >= TAM_BUFFER){
			return -1;
        }
		b_wr = write(this->sock, msg, strlen(msg));
		if (b_wr != strlen(msg)){
			return -1;
        }
	}

	return b_wr;
	
}


int Usuario::esperarEntrada(){
	int lectura_incompleta = FALSE;
	int i = 0, b_read;
	char chr;

	do{
		//Lee un caracter a la vez.
		b_read = read(this->sock, &chr, 1);
		

		//Si hay un error
		if (b_read == -1){
			lectura_incompleta = TRUE;
			this->buf[i] = '\0';
			break;
		}
		//Chequea
		if ( chr == '\r' ){
			if ( read(this->sock, &chr, 1) > 0 ){
				if (chr == '\n'){
                    
					this->buf[i] = '\0';
					this->buf[i+1] = '\0';						
					break;
                    
				}
			}else{
				return -1;
            }
		}
		//Guarda el caracter
		this->buf[i++] = chr;

	/*Se repite el loop hasta que CR-LF es leido o 
	el mensaje se vuelve demasiado largo*/
	}while(chr != '\0' && i < TAM_BUFFER-1);

	if (lectura_incompleta){
		return -1;
    }else{
		return i;
    }
}


int Usuario::parsearEntrada(){
	
	int i = 0;
	char *tmp_buf = this->buf;	

	if ( tmp_buf == NULL ){
		return -1;
    }

	if ( *tmp_buf == ':' ){
		tmp_buf = strchr(tmp_buf, ' ') + 1;
    }

	do{
		this->cmd_parametros[i] = tmp_buf;
        
		if (*tmp_buf == ':'){
			this->cmd_parametros[i++] = tmp_buf+1;
			break;
		}

		if ( ( tmp_buf = strchr(tmp_buf, ' ') ) != NULL ){
			*tmp_buf++ = '\0';
        }
        
		i++;
	}while( tmp_buf != NULL );

	this->cmd_parametros[i] = NULL;

	return i-1;
}


int Usuario::estaRegistrado(){
	return (this->estado == REG);
}

void Usuario::enviarError(int err_num, char * par, char * msg){
	char *parametros[4];
	char cmd[20] = { '\0' };
	char buffer[TAM_BUFFER];

	if ( this->estaRegistrado() ){
		parametros[0] = this->nickname;
    }else{
		parametros[0] = "*";
    }
	
	parametros[1] = par;
	parametros[2] = msg;
	parametros[3] = NULL;
	sprintf(cmd,"%d\0", err_num);	

	construirMensaje(NULL, NULL, serv_hname, cmd, parametros, buffer);

	this->enviarMensaje(buffer);

	
}


int Usuario::act(int num_parametros){
	char *cmd = this->cmd_parametros[0];
	char buffer[TAM_BUFFER];
	char *prms[MAX_PARAMETROS];

	if ( cmd == NULL ){
        return -1;
    }
	
	if ( ! strcmp(cmd, "NICK") ){
		if (num_parametros < 1 ){
			this->enviarError( INGRESANICKNAME, "" ,"No has ingresado un nickname" );	
			return -1;
		}

		if ( ! nombreUsuarioValido(this->cmd_parametros[1]) ){	
			this->enviarError( ERR_ERRONEUSNICKNAME, this->cmd_parametros[1] ,"Nickname no valido" );
			return -1;
		}

		if ( nicknameOcupado(this->cmd_parametros[1]) ){	
			this->enviarError( NICKNAMEENUSO, this->cmd_parametros[1] , "Nickname ocupado" );
			return -1;
		}
	
		this->setNickname(this->cmd_parametros[1]);
		
		switch (this->getEstado()){
			case REG:
				break;
			case NICK:
				break;
			case USER:
				this->setEstado(REG);
				this->enviarMotd();
				break;
			case NONE:
				this->setEstado(NICK);
				break;
			default:
				break;
		}
		
	}else if ( ! strcmp(cmd, "USER") ){
        
		if ( num_parametros < 4 ){
			this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
			return -1;
		}

		switch (this->getEstado()){
			case REG: case USER:
				this->enviarError( YAESTAREGISTRADO, "" ,"Ya se encuentra registrado" );
				return -1;
				break;
			case NICK:
				this->setEstado(REG);
				this->enviarMotd();
				break;
			case NONE:
				this->setEstado(USER);
				break;
			default:
				break;
		}
		
		this->setNombreUsuario(this->cmd_parametros[1]);
		this->setNombreReal(this->cmd_parametros[4]);
		
	}else{
        
		if ( !this->estaRegistrado() ){
			this->enviarError( NOESTASREGISTRADO, "" , "No estas registrado" );
			return -1;
		}
		
		if ( ! strcmp(cmd, "QUIT") ){
			return 1;
            
		}else if ( ! strcmp(cmd, "JOIN") ){
			int cid; 

			if ( num_parametros < 1 ){
				this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
				return -1;
			}
			
			
			if ( ( cid = obtenerCanalxNombre(this->cmd_parametros[1]) ) < 0 ){
				printf("Creando nuevo canal - %s\n", this->cmd_parametros[1]);
				cid = obtenerCanalSinUsar();
				if (cid < 0){
					printf("Sala del canal repleta\n");
					return -1;
				}
				canales[cid] = new Canal(this->cmd_parametros[1]);
			}

			if ( this->join(cid) < 0){
				return -1;
            }

			sprintf(buffer,":%s!%s@%s JOIN :%s\r\n", this->nickname, this->nombre_usuario, this->nombre_host, this->cmd_parametros[1]);
			canales[cid]->enviarMensaje(getUsuarioxNombre(this->nickname), buffer, TRUE);
			
			char tema[MAX_TAM_TEMA+1];
			canales[cid]->getTema(tema);
			sprintf(buffer,":%s 332 %s %s :%s\r\n", serv_hname, this->nickname, this->cmd_parametros[1], tema);
			this->enviarMensaje(buffer);
			
			char tmp[MAX_TAM_NICKNAME];
			
			sprintf(buffer, ":%s 353 %s = %s :", serv_hname, this->nickname, this->cmd_parametros[1]);
			for ( int i = 0; i < canales[cid]->contarUsuarios(); i++){
				usuarios[canales[cid]->getUsuario(i)]->getNickname(tmp);
				strcat(buffer, tmp);
				strcat(buffer, " ");
			}
			strcat(buffer,"\r\n");
			this->enviarMensaje(buffer);
			
			sprintf(buffer, ":%s 366 %s %s :End of /NAMES list\r\n", serv_hname, this->nickname, this->cmd_parametros[1]);
			this->enviarMensaje(buffer);
			
		}else if ( ! strcmp(cmd, "PART") ){
            
			int cid, num_usuarios;
	
			if ( num_parametros < 1 ){
				this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
				return -1;
			}
			
			if ( ( cid = obtenerCanalxNombre(this->cmd_parametros[1]) ) < 0 ){
				return -1;
            }
			
			sprintf(buffer, ":%s!%s@%s PART %s\r\n", this->nickname, this->nombre_usuario, this->nombre_host, this->cmd_parametros[1]);
			canales[cid]->enviarMensaje(getUsuarioxNombre(this->nickname), buffer, TRUE);

			if ( (num_usuarios = this->part(cid)) < 0 ){
				return -1;
            }

			if ( num_usuarios == 0 )	{
				delete canales[cid];
				canales[cid] = NULL;
			}	
		
		}else if ( ! strcmp(cmd, "MODE") ){

			if ( num_parametros < 1){
				
				this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
				return -1;
			}

			if ( num_parametros == 1 ){
                
				sprintf(buffer, ":%s 324 %s %s +\r\n", serv_hname, this->nickname, this->cmd_parametros[1] );
				this->enviarMensaje(buffer);
                
			}else if ( *(this->cmd_parametros[2]) == 'b' ){
                
				sprintf(buffer, ":%s 368 %s %s :Fin de la lista de baneados del canal\r\n", serv_hname, this->nickname, this->cmd_parametros[1]);
				this->enviarMensaje(buffer);
                
			}else if ( ! strcmp(this->cmd_parametros[2], "+t") ){
                
				sprintf(buffer, ":%s 482 %s %s :Usted no es el operador del canal\r\n", serv_hname, this->nickname, this->cmd_parametros[1]);
				this->enviarMensaje(buffer);
                
			}else{
                
				this->enviarError( ERR_CHANOPPRIVSNEEDED, this->cmd_parametros[1] ,"No eres es operador del canal");
				return -1;
			}

		}
		else if ( ! strcmp(cmd, "TOPIC") ){
            
			if ( num_parametros < 1 ){
				this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
				return -1;
                
			}else if ( num_parametros == 1 ){
                
				if ( ! existeCanal(this->cmd_parametros[1]) ){
					this->enviarError( NOEXISTECANAL, this->cmd_parametros[1] ,"No existe canal" );
					return -1;
				}

				//Obtiene el tópico y lo envía (primero chequea si existe el tópico)
			}else if ( num_parametros == 2 ){
				//Revisa si el usuario está en el canal y asigna el topico, manda ":nombre_host 332 nombre_usuario #channel :new topic"(or send ERR_NOTONCHANNEL)
				//Si el último parámetro está vacío, elimina el tópico
			}
		}
		else if ( ! strcmp(cmd, "NAMES") ){

			if ( num_parametros < 1 ){
				this->enviarError( FALTANPARAMETROS, cmd ,"Faltan parametros" );
				return -1;
			}
			
			/* Revisa si el usuario está en el canal y envía ":nombre_host 353 nombre_usuario = #channel :user1 user2 user3" y
			 ":nombre_host 366 nombre_usuario #channel :End of /NAMES list */

			//Si el usuario no está en el canal, manda una lista vacía de usuarios.
		}else if ( ! strcmp(cmd, "LIST") ){
			
            
		}else if ( ! strcmp(cmd, "PRIVMSG") ){
            
			if ( num_parametros == 0 ){	
                
				this->enviarError( NOEXISTERECEPTOR, cmd ,"No existe receptor" );
				return -1;
                
			}else if ( num_parametros == 1 ){	
                
				this->enviarError( INGRESEALGOPARAENVIAR, cmd ,"No hay nada para enviar" );
				return -1;
			}
            
            /*Al canal*/
			if ( *(this->cmd_parametros[1]) == '#' ){
                
				int r_cid = obtenerCanalxNombre(cmd_parametros[1]);
				if (r_cid < 0){
					this->enviarError( NOEXISTENICKNAME, cmd ,"No existe nick/canal" );
					return -1;
				}

				if ( ! this->isIn(r_cid) ){
                    
					this->enviarError( ERR_CANNOTSENDTOCHAN, cmd_parametros[1] ,"No se puede enviar al canal" );
					return -1;
				}
				prms[0] = this->cmd_parametros[1];
				prms[1] = this->cmd_parametros[2];
				prms[2] = NULL;
				construirMensaje(this->nickname, this->nombre_usuario, this->nombre_host, cmd, prms, buffer);
				canales[r_cid]->enviarMensaje(getUsuarioxNombre(this->nickname), buffer, FALSE);
                
            /* Al usuario */    
			}else{
                
				int r_uid = getUsuarioxNombre(cmd_parametros[1]);
				if (r_uid < 0){
                    
					this->enviarError( NOEXISTENICKNAME, cmd ,"No existe nick/canal" );
					return -1;
				}
				prms[0] = this->cmd_parametros[1];
				prms[1] = this->cmd_parametros[2];
				prms[2] = NULL;
				construirMensaje(this->nickname, this->nombre_usuario, this->nombre_host, cmd, prms, buffer);
				usuarios[r_uid]->enviarMensaje(buffer);


			}
		}else if ( ! strcmp(cmd, "NOTICE") ){
			
		}else if ( ! strcmp(cmd, "WHO") ){
			
		}else{
			this->enviarError( COMANDODESCONOCIDO, cmd ,"Comando desconocido" );
			return -1;
		}
	}

	return 0;
}

int Usuario::enviarMotd(){
	char msg[TAM_BUFFER];
	char *prms[3];
	

	sprintf(msg,":%s 375 %s :-%s Mensaje del Dia-\r\n", serv_hname, this->nickname, serv_hname );
	this->enviarMensaje(msg);

	sprintf(msg,":%s 372 %s :Bienvenido a nuestro IRC-server Espol\r\n", serv_hname, this->nickname);
	this->enviarMensaje(msg);

	sprintf(msg,":%s 376 %s :End of /MOTD command\r\n", serv_hname, this->nickname);
	this->enviarMensaje(msg);
}


/* Funciones relacionadas a Usuario */

int usuarioValido(int id){
	return (usuarios[id] != NULL);
}

int getUsuarioxNombre(char * nombre_r){
	char nick[MAX_TAM_NICKNAME+1];
	for (int i = 0; i < MAX_NUM_USUARIOS; i++){
		if (usuarioValido(i)){
            
			usuarios[i]->getNickname(nick);
			if (strcmp(nick, nombre_r) == 0){
				return i;
            }
		}
	}
	return -1;
}

int getUsuarioSinUsar(){
	for (int i = 0; i < MAX_NUM_USUARIOS; i++){
        
		if ( !usuarioValido(i)){
			return i;
        }
	}
	return -1;
}

int nicknameOcupado(char * nombre_r){
	return (getUsuarioxNombre(nombre_r) >= 0 );
}
//Retorna un booleano que determina si el nombre escogido es válido en el IRC
int nombreUsuarioValido(char * nombre_usu){
	int i,k;
	char valid_chrs[] = { '-', '[', ']', '\\', '\'', '^', '}', '{' };
	int ok = FALSE;

	for (i = 0; i < strlen(nombre_usu); i++){
		if ( ! isalnum(nombre_usu[i]) ){
			if (isspace(nombre_usu[i])){
                return FALSE;
            }

			for (k = 0; k < 8; k++){
				if (nombre_usu[i] == valid_chrs[k]){
					ok = TRUE;
					break;
				}
			}
			if ( ok == FALSE ){
				return FALSE;
            }
		}
		
		ok = TRUE;
	}

	return ok;
}

//Arma un mensaje utilizando varios parametros como el nickname del usuario, el comando y el texto a enviar

void construirMensaje(char *nick, char *nombre_usu, char *nombre_h, char *cmd, char *parametros[MAX_PARAMETROS], char *buf){
	int i = 0;

	memset(buf, 0, TAM_BUFFER);

	if (nombre_usu != NULL && nick != NULL){
        
		sprintf(buf, ":%s!%s@%s %s", nick, nombre_usu, nombre_h, cmd);
    }else{
        
		sprintf(buf, ":%s %s", nombre_h, cmd);
    }

	while (parametros[i] != NULL){
        
		strcat(buf, " ");
		if (parametros[i+1] == NULL){
			strcat(buf, ":");
        }

		strcat(buf, parametros[i++]);
	}
	
	strcat(buf, "\r\n");

}

