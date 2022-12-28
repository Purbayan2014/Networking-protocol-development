#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcli.h>

// Data structure for a node in the network topology
struct node {
  char name[32];
  struct node *next;
};

// Data structure for a link in the network topology
struct link {
  struct node *node1;
  struct node *node2;
  struct link *next;
};

// Head of the linked list of nodes in the network topology
struct node *node_list = NULL;

// Head of the linked list of links in the network topology
struct link *link_list = NULL;

char message_buffer[1024];
int buffer_index = 0;

void my_print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  buffer_index += vsnprintf(message_buffer + buffer_index, sizeof(message_buffer) - buffer_index, format, args);
  va_end(args);
}


// Function to add a new node to the network topology
char* add_node(struct cli_def *cli, const char *command, char *argv[], int argc) {
  if (argc < 2) {
    my_print("Usage: add more <name>");
	cli_print(cli, "Usage: add node <name>");
    return message_buffer;
  }

  // Allocate memory for the new node
  struct node *new_node = malloc(sizeof(struct node));
  if (new_node == NULL) {
	my_print("Error: Failed to allocate memory for new node");
    cli_print(cli, "Error: Failed to allocate memory for new node");
    return message_buffer;
  }

  // Initialize the new node
  strncpy(new_node->name, argv[1], 32);
  new_node->next = node_list;
  node_list = new_node;

  my_print("Added node '%s'", new_node->name);
  cli_print(cli, "Added node '%s'", new_node->name);

  return message_buffer;
}

// Function to add a new link to the network topology
char* add_link(struct cli_def *cli, const char *command, char *argv[], int argc) {
  if (argc < 3) {
    cli_print(cli, "Usage: add link <node1> <node2>");
    return CLI_ERROR;
  }

  // Find the two nodes in the network topology
  struct node *node1 = NULL;
  struct node *node2 = NULL;
  struct node *current_node = node_list;
  while (current_node != NULL) {
    if (strcmp(current_node->name, argv[1]) == 0) {
      node1 = current_node;
    }
    if (strcmp(current_node->name, argv[2]) == 0) {
      node2 = current_node;
    }
    current_node = current_node->next;
  }

  if (node1 == NULL) {
	my_print("Error: Node '%s' not found", argv[1]);
    cli_print(cli, "Error: Node '%s' not found", argv[1]);
    return message_buffer;
  }
  if (node2 == NULL) {
	my_print("Error: Node '%s' not found", argv[2]);
    cli_print(cli, "Error: Node '%s' not found", argv[2]);
    return message_buffer;
  }

  // Allocate memory for the new link
  struct link *new_link = malloc(sizeof(struct link));
  if (new_link == NULL) {
	my_print("Error: Failed to allocate memory for new link");
    cli_print(cli, "Error: Failed to allocate memory for new link");
    return message_buffer;
  }

  // Initialize the new link
  new_link->node1 = node1;
  new_link->node2 = node2;
  new_link->next = link_list;
  link_list = new_link;
    
  my_print("Added link between '%s' and '%s' nodes",new_link->node1->name, new_link->node2->name);
  cli_print(cli, "Added link between '%s' and '%s' nodes",new_link->node1->name, new_link->node2->name);
  return message_buffer;
}

// Function to view the current network topology
char* view_topology(struct cli_def *cli, const char *command, char *argv[], int argc) {
  cli_print(cli, "Nodes:");
  my_print("Nodes: \n");
  struct node *current_node = node_list;
  while (current_node != NULL) {
    my_print(cli, "  %s", current_node->name);
    current_node = current_node->next;
  }

  cli_print(cli, "Links:");
  my_print("Links: \n");
  struct link *current_link = link_list;
  while (current_link != NULL) {
    my_print(" %s  <-----> %s\n", current_link->node1->name, current_link->node2->name);
    cli_print(cli, "  %s <-> %s", current_link->node1->name, current_link->node2->name);
    current_link = current_link->next;
  }

  return message_buffer;
}

int main(int argc, char **argv) {
	  // Create a new libcli context
	  struct cli_def *cli = cli_init();
	  cli_set_banner(cli, "Network Topology Manager");

	  // Register the "add node" command
	  cli_register_command(cli, NULL, "add node", add_node, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Add a new node to the network");

	// Register the "add link" command
	cli_register_command(cli, NULL, "add link", add_link, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Add a new link between two nodes in the network topology");

	// Register the "view" command
	cli_register_command(cli, NULL, "view", view_topology, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "View the current network topology");

	// Loop indefinitely, reading commands from stdin and processing them
	while (1) {
	  
      char input_buffer[1024];
	  printf("> ");
	  fgets(input_buffer, sizeof(input_buffer), stdin);
      
      if (input_buffer == "add node") {
        char *result1 = add_node(cli, "add node", NULL, 0);
        printf(result1);
      }
      else if (input_buffer == "add link") {
        char *result2 = add_link(cli, "add link", NULL, 0);
        printf(result2);
      } else if (input_buffer == "view") {
        char *result3 = view_topo(cli, "view", NULL, 0);
        printf(result3);
      } else {
			break;
		}
	}

// Free the libcli context
cli_done(cli);

return 0;
}
