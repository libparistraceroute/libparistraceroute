#include "fieldlist.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

fieldlist_s* add_fieldlist(fieldlist_s* list_of_list, char* field_name, field_s* list){
	if(check_presence_fieldlist(field_name,list_of_list)==1){
		#ifdef DEBUG
		fprintf(stdout,"add fieldlist : there's already a field list with this name : %s. fieldlist replaced\n", field_name);
		#endif
		set_fieldlist_value(list_of_list,field_name,list);
		return list_of_list;
		
	}
	fieldlist_s* new = malloc(sizeof(fieldlist_s));
	new->name=malloc((strlen(field_name)+1)*sizeof(char));
	memcpy(new->name,field_name,strlen(field_name)+1);
	new->value=list;
	new->next = list_of_list;
	return new;
}


fieldlist_s* create_fieldlist(char* field_name, field_s* first_list){
	fieldlist_s* res = add_fieldlist(NULL, field_name,first_list);
	return res;
}


void view_fieldlist(fieldlist_s* fields){
	fieldlist_s* cur_field=fields;
	while(cur_field!=NULL){
		fprintf(stdout,"pour la liste de field %s : \n",cur_field->name);
		view_fields((field_s*)cur_field->value);
		cur_field=cur_field->next;
	}
}

int set_fieldlist_value(fieldlist_s* fields, char* field_name, field_s* new_value){
	fieldlist_s* current = fields;
	while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
		current = current->next;
	}
	//soit dernier, soit on a trouvé
	if(strcasecmp(current->name,field_name)==0){
		if(current->value!=NULL){
			free_all_fields(current->value, 1);//we free the content of fields that will be replaced TODO check
		}
		current->value = new_value;
		return 1;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"error : field %s not found in fields list when trying to update value\n",field_name);
		#endif
		return 0;
	}
}

int update_fieldlist_value(fieldlist_s* fields, char* field_name, field_s* new_value){
	fieldlist_s* current = fields;
	while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
		current = current->next;
	}
	//soit dernier, soit on a trouvé
	if(strcasecmp(current->name,field_name)==0){
		if(current->value!=NULL){
		}
		current->value = new_value;
		return 1;
	}
	else{
		#ifdef DEBUG
		fprintf(stderr,"error : field %s not found in fields list when trying to update value\n",field_name);
		#endif
		return 0;
	}
}
void* get_field_value_from_fieldlist(fieldlist_s* fields, char* field_name){
	fieldlist_s* current = fields;
	void* res = NULL;
	while(current!=NULL && res==NULL){
		res = get_field_value(current->value, field_name);
		current = current->next;
	}
	//si trouvé, found !=NULL
	return res;
}

field_s* get_fieldlist_value(fieldlist_s* fields, char* field_name){
	fieldlist_s* current = fields;
	if(fields==NULL){
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
		#ifdef DEBUG
		fprintf(stderr,"warning : field list %s not found in fields list when trying to get value\n",field_name);
		#endif
		return NULL;
	}
}


void free_fieldlist(fieldlist_s* fields, int with_content){
	fieldlist_s* cur=fields;
	if(with_content!=0){
		free_all_fields(cur->value, with_content);
	}
	free(cur->name);
	//free(cur);
	return;
}

void free_all_fieldlists(fieldlist_s* fields, int with_content){
	fieldlist_s* cur=fields;
	fieldlist_s* next;
	while(cur!=NULL){
		free_all_fields(cur->value, with_content);
		free(cur->name);
		next=cur->next;
		free(cur);
		cur = next;
	}
	return;
}

void free_all_fieldlists_with_function(fieldlist_s* fields, void (*free_func)(void* elem)){
	fieldlist_s* cur=fields;
	fieldlist_s* next;
	while(cur!=NULL){
		free_all_fields_with_function(cur->value, free_func);
		free(cur->name);
		next=cur->next;
		free(cur);
		cur = next;
	}
	return;
}

int remove_fieldlist(fieldlist_s** fields_p, char* field_name,int with_content){
	if(fields_p==NULL||field_name==NULL){
		#ifdef DEBUG
		fprintf(stderr,"remove fieldlist : empty parameters\n");
		#endif
		return -1;		
	}
	fieldlist_s* fields =*fields_p;
	if(check_presence_fieldlist(field_name,fields)==-1){
		#ifdef DEBUG
		fprintf(stderr,"remove fieldlist : fieldlist %s not found in fieldlists\n",field_name);
		#endif
		return -1;
	}
	if(strcasecmp(fields->name,field_name)==0){//it's the first element
		fieldlist_s* cur = fields->next;
		free_fieldlist(fields, with_content);
		*fields_p=cur;
		return 1;
	}
	else{
		fieldlist_s* current = fields;
		fieldlist_s* prev = fields;
		while(current->next!=NULL && strcasecmp(current->name,field_name)!=0){
			prev=current;
			current = current->next;
		}
		if(strcasecmp(current->name,field_name)==0){//can't be 'else'
			prev->next=current->next;
			free_fieldlist(current, with_content);
			return 1;
		}
		else{
			#ifdef DEBUG
			fprintf(stderr,"remove fieldlist : this case is normally impossible\n");
			#endif
			return -1;
		}
	}
	return 1;
}

int remove_field_from_fieldlist(fieldlist_s** fields, char* field_name,char* fieldlist_name, int with_content){
	if(fields==NULL||field_name==NULL){
		#ifdef DEBUG
		fprintf(stderr,"remove_field_from_fieldlist : empty parameters\n");
		#endif
		return -1;
	}
	field_s** fields_list;
	if(fieldlist_name!=NULL){
		fieldlist_s* current = *fields;
		while(current->next!=NULL && strcasecmp(current->name,fieldlist_name)!=0){
			current = current->next;
		}
		//soit dernier, soit on a trouvé
		if(strcasecmp(current->name,fieldlist_name)==0){
			fields_list = &current->value;
			
		}
		else{
			#ifdef DEBUG
			fprintf(stderr,"remove_field_from_fieldlist : field list %s not found in fields list when trying to remove %s \n",fieldlist_name, field_name);
			#endif
			return -1;
		}
		//we remove the field now
		if(remove_field(fields_list,field_name, with_content)==-1){
			#ifdef DEBUG
			fprintf(stderr,"remove_field_from_fieldlist : removing of field  %s from fieldlist %s failed\n", field_name, fieldlist_name);
			#endif
			return -1;	
		}
		return 1;
	}
	else{//fieldlist = all
		fieldlist_s* current = *fields;
		int res=-1;
		while(current!=NULL){
			if(remove_field_from_fieldlist(fields,field_name,current->name,with_content)==1){
				res=1;
			}
			current = current->next;
		}
		return res;
	}
}
int check_presence_fieldlist(char* fieldlist_name,fieldlist_s* fields){
	fieldlist_s* next_el=fields;
	while(next_el!=NULL&& strcmp(next_el->name, fieldlist_name)!=0 ){
		next_el=next_el->next;
	}
	if (next_el==NULL){
		return -1;
	}
	return 1;		
}

int check_presence_list_fieldlist(char** fieldlists_names, int size, fieldlist_s* fields){
	int i =0;
	int res =1;
	for(i=0;i<size;i++){
		if(check_presence_fieldlist(fieldlists_names[i],fields)==-1){
			#ifdef DEBUG
			fprintf(stderr,"warning : fieldlist %s not found in fieldlists\n",fieldlists_names[i]);//TODO : print anytime?
			#endif
			res = -1;
		}
	}
	return res;
}

int check_presence_field(char* field_name,fieldlist_s* fieldlist){
	fieldlist_s* next_el=fieldlist;
	int is_found = -1;
	while(next_el!=NULL && is_found!=1 ){//next_el : the fieldlist
		is_found=check_presence(field_name, next_el->value);
		next_el=next_el->next;
	}
	if (next_el==NULL && is_found == -1){
		return -1;
	}
	return 1;		
}

int check_presence_list_of_fields(char** fields_names,int size, fieldlist_s* fieldlist){
	int i =0;
	int res =1;
	for(i=0;i<size;i++){
		//fprintf(stdout,"checking %s\n",fields_names[i]);
		if(check_presence_field(fields_names[i],fieldlist)==-1){
			#ifdef DEBUG
			fprintf(stderr,"warning : field %s not found in fieldlists\n",fields_names[i]);//TODO : print anytime?
			#endif
			res = -1;
		}
	}
	return res;	
}

int get_number_of_fieldlists(fieldlist_s* fields){
	int res=0;
	fieldlist_s* current = fields;
	if(fields==NULL){
		return 0;
	}
	while(current!=NULL){
		res++;
		current=current->next;
	}
	return res;
}

