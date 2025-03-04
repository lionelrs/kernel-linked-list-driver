#include <linux/module.h>    // Required for all kernel modules
#include <linux/init.h>      // Macros for module initialization and cleanup
#include <linux/slab.h>      // Memory allocation (kmalloc, kfree)
// #include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("Simple character device module");
MODULE_VERSION("1.0");

struct node {
    char *data;
    struct node *next;
};

static struct node *head = NULL;

static void push_front(const char *data) {
    struct node *new_node = kmalloc(sizeof(struct node), GFP_KERNEL);
    if (!new_node) {
        printk(KERN_ERR "Memory allocation failed for new node\n");
        return;
    }

    new_node->data = kstrdup(data, GFP_KERNEL);
    new_node->next = head;
    head = new_node;
}

static void push_back(const char *data) {
    struct node *new_node = kmalloc(sizeof(struct node), GFP_KERNEL);
    if (!new_node) {
        printk(KERN_ERR "Memory allocation failed for new node\n");
        return;
    }

    new_node->data = kstrdup(data, GFP_KERNEL);
    new_node->next = NULL;

    if (!head) {
        head = new_node;
        return;
    }
    struct node *tmp = head;
    while (tmp->next) {
        tmp = tmp->next;
    }
    tmp->next = new_node;
}

static void pop_front(void) {
    if (!head) {
        return;
    }
    struct node *tmp = head;
    head = head->next;
    kfree(tmp->data);
    kfree(tmp);
}

static void delete_list(void) {
    while (head) {
        pop_front();
    }
}

static void display_list(void) {
    struct node *tmp = head;
    while (tmp) {
        printk(KERN_INFO "%s\n", tmp->data);
        tmp = tmp->next;
    }
}

static int __init chardev_init(void) {
    printk(KERN_INFO "Linked list character device loaded successfully\n");
    return 0;
}

static void __exit chardev_exit(void) {
    printk(KERN_INFO "Linked list character device unloaded successfully\n");
}

module_init(chardev_init);
module_exit(chardev_exit);