#include "field.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>


field_s* add_field(field_s* next, char* field_name, void* new_value){
	if(check_presence(field_name,next)==1){
		#ifdef DEBUG
		fprintf(stdout,"add field : there's already a field with this name : %s. field updated (old freed)\n", field_name);
		#endif
		set_field_value(next,field_name,new_value,1);
		return next;
		
	}
	field_s* new = malloc(sizeof(field_s));
	new->name=malloc((strlen(field_name)+1)*sizeof(char));
	memcpy(new->name,field_name,strlen(field_name)+1);
	new->value=new_value;
	new->next = next;
	return new;
}

field_s* add_int_field(field_s* next, char* field_name, int new_value){
	int* res = malloc(sizeof(int));
	*res=new_value;
	return add_field(next,field_name, res);
}


field_s* add_short_field(field_s* next, char* field_name, short new_value){
	short* res = malloc(sizeof(short));
	*res=new_value;
	return add_field(next,field_name, res);
}


field_s* add_char_field(field_s* next, char* field_name, char new_value){
	char* res = malloc(sizeof(char));
	*res=new_value;
	return add_field(next,field_name, res);
}


field_s* add_string_field(field_s* next, char* field_name, char* new_value){
	char* res = malloc((strlen(new_value)+1)*sizeof(char));
	strncpy(res,new_value,strlen(new_value)+1);
	return add_field(next,field_name, res);
}


field_s* add_float_field(field_s* next, char* field_name, float new_value){
	int* res = malloc(sizeof(int));
	*res=new_value;
	return add_field(next,field_name, res);
}


field_s* add_field_end(field_s* before, char* field_name, void* new_value){
	if(check_presence(field_name,before)==1){
		#ifdef DEBUG
		fprintf(stdout,"add field : there's already a field with this name : %s. field updated (old freed)\n", field_name);
		#endif
		set_field_value(before,field_name,new_value,1);
		return before;
		
	}
	field_s* new = malloc(sizeof(field_s));
	new->name=malloc((strlen(field_name)+1)*sizeof(char));
	memcpy(new->name,field_name,strlen(field_name)+1);
	new->value=new_value;
	new->next = NULL;
	if(before == NULL){
		return new;
	}
	field_s* go = before;
	while(go->next!=NULL){
		go=go->next;
	}
	go->next = new;
	return before;
}

field_s* create_field(char* field_name, void* new_value){
	field_s* res = add_field(NULL,field_name,new_value);
	return res;
}

/*
field_s* create_empty_fields_from_array(char** array, int size){
	if(size<=0){
		#ifdef DEBUG
		fprintf(stderr,"create_fields_from_array : trying to create from empty array.end\n");
		#endif
		return NULL;
	}
	else{
		field_s* res = create_field(array[0],NULL);	
	}
}*/

field_s* get_next_field(field_s* fields){
	if(fields==NULL){
		return NULL;
	}
	return fields->next;
}

void view_fields(field_s* fields){
	int i;
	int stop;
	field_s* cur_field=fields;
	while(cur_field!=NULL){
		stop=0;
		i=0;
		//char res;
		//fprintf(stdout,"field %s has for value %s or %i\n",cur_field->name,*((char**)cur_field->value),*((int*)cur_field->value));
		//fprintf(stdout,"field %s has for value (begin) %c%c%c or %i\n",cur_field->name,*(*(char**)cur_field->value),*(*((char**)cur_field->value)+1),*(*((char**)cur_field->value)+2),*((int*)cur_field->value));
		fprintf(stdout,"field %s has for value %d or %d or %d or string \"",cur_field->name, *(unsigned char*)cur_field->value, *((unsigned short*)cur_field->value), *((unsigned int*)cur_field->value));//TODO : string
		char res;
		while(stop==0){
			res = *((char*)cur_field->value+i);
			if( res >= 'A' &&  res <= 'z' ){
				fprintf(stdout,"%c", res);
				i++;
			}
			else{
				fprintf(stdout, "\"\n");
				stop=1;
			}
		}
		
		//fprintf(stdout," a string\n");
		cur_field=cur_field->next;
	}
	fprintf(stdout,"end of fields\n");
}

void view_fields_with_function(field_s* fields, void (*function) (void* field) ){

	int i;
	int stop;
	field_s* cur_field=fields;
	while(cur_field!=NULL){
		stop=0;
		i=0;
		function(cur_field->value);
		cur_field=cur_field->next;
	}
	fprintf(stdout,"end of fields\n");
}

int set_field_value(field_s* fields, char* field_name, void* new_value, int delete_old){
	field_s* current = fields;
	while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
		//printf("current : %s we look for : %s\n",current->name,field_name);
		current = current->next;
	}
	//soit dernier, soit on a trouvé
	if(strcasecmp(current->name,field_name)==0){
		if(current->value!=NULL){
			if(delete_old!=0){
				free(current->value);
			}
		}
		current->value = new_value;
		return 1;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"set_field_value : field %s not found in fields list when trying to update value\n",field_name);
		#endif
		return 0;
	}
}



void* get_field_value(field_s* fields, char* field_name){
	field_s* current = fields;
	if(current== NULL){
		return NULL;
	}
	while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
		current = current->next;
	}
	//soit dernier, soit on a trouvé
	if(strcasecmp(current->name,field_name)==0){
		return current->value;
	}
	else{
		return NULL;
	}
}

char* get_field_name(field_s* fields){
	if(fields==NULL){
		#ifdef DEBUG
		fprintf(stderr,"get_field_name : fields parameter is empty\n");
		#endif
		return NULL;
	}
	return fields->name;
}

int remove_field(field_s** fields_p, char* field_name, int with_content){
	if(fields_p==NULL||field_name==NULL){
		#ifdef DEBUG
		fprintf(stderr,"remove field : empty parameters\n");
		#endif
		return -1;
	}
	field_s* fields = * fields_p;
	if(check_presence(field_name,fields)==-1){
		#ifdef DEBUG
		fprintf(stderr,"remove field : field %s not found in field list\n",field_name);
		#endif
		return -1;
	}
	if(strcasecmp(fields->name,field_name)==0){//it's the first element
		field_s* cur = fields->next;
		free_field(fields,with_content);
		*fields_p=cur;		
		return 1;
	}
	else{
		field_s* current = fields;
		field_s* prev = fields;
		while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
			prev=current;
			current = current->next;
		}
		if(strcasecmp(current->name,field_name)==0){//can't be 'else'
			prev->next=current->next;
			free_field(current, with_content);
			return 1;
		}
		else{
			#ifdef DEBUG
			fprintf(stderr,"remove field: this case is normally impossible\n");
			#endif
			return -1;
		}
	}
	return 1;
}
void free_all_fields(field_s* fields, int with_content){
	field_s* cur=fields;
	field_s* next;
	while(cur!=NULL){
		if(with_content!=0){
			free(cur->value);
		}
		free(cur->name);
		next=cur->next;
		free(cur);
		cur = next;
	}
	return;
}

void free_all_fields_with_function(field_s* fields, void (*free_func)(void* ele)){
	field_s* cur=fields;
	field_s* next;
	while(cur!=NULL){
		free_func(cur->value);
		free(cur->name);
		next=cur->next;
		free(cur);
		cur = next;
	}
	return;
}

void free_field(field_s* fields, int with_content){
	field_s* cur=fields;
		if(with_content!=0){
			free(cur->value);
		}
		free(cur->name);
		free(cur);
	return;
}

int check_presence(char* field_name,field_s* fields){
	field_s* next_el=fields;
	while(next_el!=NULL&& strcmp(next_el->name, field_name)!=0 ){
		next_el=next_el->next;
	}
	if (next_el==NULL){
		return -1;
	}
	return 1;		
}

int check_presence_list(char** fields_names, int size, field_s* fields){
	int i =0;
	int res =1;
	for(i=0;i<size;i++){
		if(check_presence(fields_names[i],fields)==-1){
			res = -1;
		}
	}
	return res;
}

int get_number_of_fields(field_s* fields){
	int res=0;
	field_s* current = fields;
	if(fields==NULL){
		return 0;
	}
	while(current!=NULL){
		res++;
		current=current->next;
	}
	return res;
}


