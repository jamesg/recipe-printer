#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
	#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#define INGREDIENT_LENGTH 25
#define SCREEN_WIDTH 80

struct list_t {
	struct ingredient_t* ingredient;
	struct list_t* next;
};

struct ingredient_t {
	struct list_t* ingredient_list;
	char* label;
	int column;
};

struct recipe_buffer_t {
	char* buffer;
	int height;
	int width;
	int ingredient_length;
};

struct ingredient_t* dish;

void* malloc_chk(size_t size) {
	void* v = 0;
	if ((v = malloc(size)) == 0) {
		fprintf(stderr, "Error allocating memory\n");
		exit(1);
	}
	return v;
}

// Allocate memory for a list item and add it to list head
// If the list is a stack, this is like pushing the item.
struct list_t* malloc_list_item(struct list_t** head) {
	struct list_t* list = malloc_chk(sizeof(struct list_t));
	(*list).ingredient = 0;
	if (head != 0) {
		(*list).next = *head;
		*head = list;
	} else {
		(*list).next = 0;
	}
	return list;
}

struct list_t* list_next(struct list_t* l) {
	if (l == 0) {
		return 0;
	}
	return (*l).next;
}

// Allocate memory for an ingredient and add it to parent
struct ingredient_t* malloc_ingredient(struct ingredient_t* parent) {
	struct ingredient_t* new_ingredient = malloc_chk(sizeof(struct ingredient_t));
	(*new_ingredient).label = 0;
	(*new_ingredient).ingredient_list = 0;
	(*new_ingredient).column = 0;

	if (parent != 0) {
		struct list_t* new_list_item = malloc_chk(sizeof(struct list_t));
		(*new_list_item).ingredient = new_ingredient;
		(*new_list_item).next = (*parent).ingredient_list;
		(*parent).ingredient_list = new_list_item;
	}

	return new_ingredient;
}

struct recipe_buffer_t* malloc_recipe_buffer(int height, int width, int ingredient_length) {
	struct recipe_buffer_t* rb = malloc_chk(sizeof(struct recipe_buffer_t));
	(*rb).buffer = malloc_chk(height* ((width*ingredient_length)+1) +1);
	memset((*rb).buffer, (int)' ',  height* ((width*ingredient_length)+1) );
	int i;
	for (i = 0; i < height; i++) {
		(*rb).buffer[ i*((width*ingredient_length)+1) + (width*ingredient_length) ] = '\n';
	}
	(*rb).buffer[height* ((width*ingredient_length)+1)] = '\0';
	(*rb).height = height;
	(*rb).width = width;
	(*rb).ingredient_length = ingredient_length;
	return rb;
}

void free_recipe_buffer(struct recipe_buffer_t* rb) {
	free((*rb).buffer);
	free(rb);
}

char* recipe_buffer_pointer(struct recipe_buffer_t* rb, int row, int column) {
	return (*rb).buffer
	+ (((*rb).width * (*rb).ingredient_length)+1)*row
	+ (column*(*rb).ingredient_length);
}

// Get the first item in the list of sub ingredients
struct ingredient_t* head(struct ingredient_t* ingredient) {
	return (*(*ingredient).ingredient_list).ingredient;
}

// Free one list item.  If the list is a stack, this is like popping the item.
struct ingredient_t* free_list_item(struct list_t** l) {
	if (*l == 0) return 0;

	struct ingredient_t* i;
	i = (**l).ingredient;

	struct list_t* t = (*(*l)).next;
	free(*l);
	(*l) = t;

	return i;
}

// Free the list of ingredients, not any strings
void free_ingredient(struct ingredient_t* ingredient) {
	struct list_t* i = (*ingredient).ingredient_list;
	while (i != 0) {
		struct list_t* n = (*i).next;
		if ((*i).ingredient != 0) {
			free_ingredient((*i).ingredient);
		}
		i = n;
	}
	free(ingredient);
}

void print_list(struct list_t* l) {
	printf("list <");
	while (l != 0) {
		printf("%s, ", (*(*l).ingredient).label);
		l = (*l).next;
	}
	printf(">\n");
}

void ddlparse(struct ingredient_t* ingredient, char** buffer) {
	enum {
		NEW_INGREDIENT, NEW_LISTPART, NEW_METHOD,
		INGREDIENT, LISTPART, METHOD
	} state = NEW_INGREDIENT;

	while (**buffer != '\0') {
		char c = **buffer;
		switch (state) {
		case NEW_INGREDIENT:
			switch (c) {
			// Compound ingredient
			case '(': {
				(char*)(*buffer)++;
				struct ingredient_t* new_ingredient = malloc_ingredient(ingredient);
				ddlparse(new_ingredient, buffer);
				break;
			}
			case '>':
			case ',':
				state = NEW_LISTPART;
				break;
			// Single ingredient
			default: {
				struct ingredient_t* new_ingredient = malloc_ingredient(ingredient);
				(*new_ingredient).label = (char*)(*buffer);
				(char*)(*buffer)++;
				state = INGREDIENT;
				break;
				}
			}
			break;
		case INGREDIENT:
			switch (c) {
			case ',':
			case '>':
				state = NEW_LISTPART;
				break;
			default:
				(char*)(*buffer)++;
				break;
			}
			break;
		case NEW_LISTPART:
			switch (c) {
			case ',':
				state = NEW_INGREDIENT;
				break;
			case '>':
				state = NEW_METHOD;
				break;
			}
			**buffer = '\0';
			(char*)(*buffer)++;
			break;
		case NEW_METHOD:
			(*ingredient).label = (char*)(*buffer);
			(char*)(*buffer)++;
			state = METHOD;
			break;
		case METHOD:
			switch (c) {
			case ')': {
				(**buffer) = '\0';
				(char*)(*buffer)++;
				return;
				break;
				}
			case '\n':
				(**buffer) = '\0';
				(char*)(*buffer)++;
				break;
			default:
				(char*)(*buffer)++;
				break;
			}
			break;
		default:
			printf("Should never be reached\n");
			break;
		}
	}
}

void print_ingredient(struct ingredient_t* ingredient) {
	if (ingredient == 0) return;
	if ((*ingredient).ingredient_list != 0) { // there are sub ingredients
		printf("c%d (", (*ingredient).column);
		// print all the sub-ingredients
		struct list_t* i = (*ingredient).ingredient_list;
		while (i != 0) {
			print_ingredient((*i).ingredient);
			if ((*i).next != 0) {
				printf(",");
			}
			i = (*i).next;
		}
		// print the label
		printf(">%s", (*ingredient).label);
		printf(")");
	} else {
		// just print the label
		printf((*ingredient).label);
	}
}

// Print up to n characters of a string.  If there are fewer than
// n characters print additional spaces.
void printn(char* s, int n) {
	int i = 0;
	// Print string up to n characters
	while (s != 0 && i < n) {
		if (s[i] == '\0') {
			break;
		}
		printf("%c", s[i]);
		i++;
	}
	// Fill remaining characters
	for (; i < n; i++) {
		printf(" ");
	}
}

void printn_border_right(char *s, int n) {
	printn(s, n-1);
	printf("|");
}

void printn_border_left(char *s, int n) {
	printf("|");
	printn(s, n-1);
}

void printn_border_both(char *s, int n) {
	printf("|");
	printn(s, n-2);
	printf("|");
}

void print_dashed_line(int n) {
	int i = 0;
	for (; i < n; i++) {
		printf("-");
	}
}

int max_depth(struct ingredient_t* ingredient) {
	int depth = 0;

	struct list_t* i = (*ingredient).ingredient_list;
	while (i != 0) {
		depth = max(depth, max_depth((*i).ingredient)+1);
		i = (*i).next;
	}
	return depth;
}

int count_basic_ingredients(struct ingredient_t* ingredient) {
	struct list_t* i = (*ingredient).ingredient_list;
	if (i == 0) {
		return 1;
	} else {
		int c = 0;
		while (i != 0) {
			c += count_basic_ingredients((*i).ingredient);
			i = (*i).next;
		}
		return c;
	}
}

char* sprintn(char* s, char* d, int n) {
	if (s == 0) return;
	int i;
	for (i = 0; i < n-1; i++) {
		if (s[i] == '\0') return &(s[i]);
		// leave a space before the label
		d[i+1] = s[i];
	}
	return &(s[i-1]);
}

void sprint_dashed(char* d, int n) {
	int i;
	for (i = 0; i < n; i++) {
		d[i] = '-';
	}
}

/*
 * Recipe printing "algorithm"
 * 
 * -------------------------------------------------
 * Aubergine   | Slice     |       |       |       |
 * -------------------------       |       |       |
 * Flour                   | Coat  |       |       |
 * ---------------------------------       |       |
 * Egg         | Whisk             | Coat  | Fry   |
 * -------------------------------------------------
 * 
 * . Move labels as far to the right as possible.
 * 
 */
void print_recipe_buffer(struct ingredient_t* ingredient, struct recipe_buffer_t* buffer, int* row) {
	// hey, look, an algorithm!
	int orig_row = *row;
	struct list_t* i;
	for (i = (*ingredient).ingredient_list; i != 0; i = (*i).next) {
		print_recipe_buffer((*i).ingredient, buffer, row);
		int ii;
		for (ii = orig_row; ii <= *row; ii++) {
			*(recipe_buffer_pointer(buffer, ii, ((*ingredient).column)) -1) = '|';
		}
		if ((*i).next != 0) {
			(*row)++;
			sprint_dashed(recipe_buffer_pointer(buffer, *row, 0), (*buffer).ingredient_length*(*ingredient).column);
			(*row)++;
		}
	}
	
	char* s = (*ingredient).label;
	int ii;
	for (ii = orig_row; ii <= *row; ii++) {
		s = sprintn(s, recipe_buffer_pointer(buffer, ii, (*ingredient).column), (*buffer).ingredient_length);
	}
}

void print_recipe(struct ingredient_t* ingredient) {
	struct recipe_buffer_t* buffer = malloc_recipe_buffer(count_basic_ingredients(ingredient)*2 +1, (*ingredient).column+1, min(INGREDIENT_LENGTH, SCREEN_WIDTH/(max_depth(ingredient)+1)));
	
	int row = 1;
	print_recipe_buffer(ingredient, buffer, &row);
	
	// right border
	int i;
	for (i = 0; i < (*buffer).height; i++) {
		*(recipe_buffer_pointer(buffer, i, ((*buffer).width)) -1) = '|';
	}

	// top, bottom borders
	sprint_dashed((*buffer).buffer, (*buffer).width*(*buffer).ingredient_length);
	sprint_dashed(recipe_buffer_pointer(buffer, (*buffer).height-1, 0), (*buffer).width*(*buffer).ingredient_length);
	
	printf((*buffer).buffer);
	
	free_recipe_buffer(buffer);
}

// Assign column numbers to ingredients
int assign_columns(struct ingredient_t* ingredient) {
	struct list_t* i;
	
	i = (*ingredient).ingredient_list;
	
	// Find the maximum height for sub ingredients
	int mh = 0;
	while (i != 0) {
		mh = max(mh, max_depth((*i).ingredient));
		i = (*i).next;
	}
	
	// Assign this height to all sub ingredients
	i = (*ingredient).ingredient_list;
	while (i != 0) {
		// basic ingredient
		if ( (*((*i).ingredient)).ingredient_list == 0) {
			(*((*i).ingredient)).column = 0;
		} else {
//			(*((*i).ingredient)).column = mh;
			assign_columns( (*i).ingredient );
		}
		i = (*i).next;
	}
	
	(*ingredient).column = mh +1;

	return (*ingredient).column;
}

int main(int argc, char* args[]) {
	if (argc > 2) {
		fprintf(stderr, "Usage: ddlparse [filename]\n");
		exit(1);
	}
	
	char* buffer = 0;
	long filesize = 0;
	if (argc == 1) {
		char tmp[1024];
		buffer = malloc_chk(1024);
		
		while (fgets(tmp,1024,stdin)) {
			filesize += strlen(tmp);
			buffer = realloc(buffer,filesize);
			strcat(buffer,tmp);
		}
	} else if (argc == 2) {
		struct stat stat_d;
		if ((stat(args[1], &stat_d)) != 0) {
			fprintf(stderr, "File not found\n");
			exit(1);
		}
		buffer = malloc_chk((stat_d.st_blocks*stat_d.st_blksize)+1);
		FILE* file = fopen(args[1], "r");
		filesize = fread(buffer,sizeof(char),(stat_d.st_blocks*stat_d.st_blksize),file);
		fclose(file);
		buffer[filesize] = 0;
	}
	
	char* working_buffer = buffer;
	struct ingredient_t* dish = malloc_ingredient(0);

	ddlparse(dish, &working_buffer);
	assign_columns(dish);
	print_recipe(dish);
	
	
	free_ingredient(dish);
	free(buffer);
	
	return 0;
}
