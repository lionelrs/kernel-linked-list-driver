/*
 * linked_list_chardev.c - A character device driver with kernel-space linked list
 *
 * Implements a simple linked list with the following operations:
 * - ADDF: Add to front of list
 * - ADDB: Add to back of list
 * - DELF: Delete from front of list
 * - DELA: Delete entire list
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DEVICE_NAME "linked_list_dev"
#define CLASS_NAME "linked_list"
#define MAX_INPUT_SIZE 256
#define MAX_LIST_SIZE 100

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel-space Linked List Character Device");
MODULE_VERSION("0.1");

// Linked List Node Structure
struct node {
    char *data;
    struct node *next;
};

// Device-specific structure
struct linked_list_dev {
    struct node *head;
    int list_size;
    struct mutex list_mutex;
    struct class *device_class;
    struct device *device;
    int major_number;
    int data_len;
};
 
// Global device instance
static struct linked_list_dev list_device;

// Function prototypes
static int dev_open(struct inode *inodep, struct file *filep);
static int dev_release(struct inode *inodep, struct file *filep);
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset);
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset);
 
// File operations structure
static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};
 
// Linked List Helper Functions
static void add_front(struct linked_list_dev *dev, const char *data) {
    struct node *new_node;

    // Check list size limit
    // if (dev->list_size >= MAX_LIST_SIZE) {
    //     printk(KERN_WARNING "List size limit reached. Cannot add more nodes.\n");
    //     return;
    // }

    // Allocate new node
    new_node = kmalloc(sizeof(struct node), GFP_KERNEL);
    if (!new_node) {
        printk(KERN_ERR "Failed to allocate memory for new node\n");
        return;
    }

    // Allocate and copy string
    new_node->data = kstrdup(data, GFP_KERNEL);
    if (!new_node->data) {
        kfree(new_node);
        printk(KERN_ERR "Failed to allocate memory for node data\n");
        return;
    }
    list_device.data_len += strlen(data) + 1;

    // Update links
    new_node->next = dev->head;
    dev->head = new_node;
    dev->list_size++;
}
 
static void add_back(struct linked_list_dev *dev, const char *data) {
    struct node *new_node, *current_node;
    // Check list size limit
    if (dev->list_size >= MAX_LIST_SIZE) {
        printk(KERN_WARNING "List size limit reached. Cannot add more nodes.\n");
        return;
    }

    // Allocate new node
    new_node = kmalloc(sizeof(struct node), GFP_KERNEL);
    if (!new_node) {
        printk(KERN_ERR "Failed to allocate memory for new node\n");
        return;
    }

    // Allocate and copy string
    new_node->data = kstrdup(data, GFP_KERNEL);
    if (!new_node->data) {
        kfree(new_node);
        printk(KERN_ERR "Failed to allocate memory for node data\n");
        return;
    }

    // Set next to NULL
    new_node->next = NULL;

    // If list is empty, set as head
    if (!dev->head) {
        dev->head = new_node;
    } else {
        // Traverse to end of list
        current_node = dev->head;
        while (current_node->next) {
            current_node = current_node->next;
        }
        current_node->next = new_node;
    }
    dev->list_size++;
    list_device.data_len += strlen(data) + 1;
}
 
static void delete_front(struct linked_list_dev *dev) {
    struct node *temp;
    // Check if list is empty
    if (!dev->head) {
        printk(KERN_INFO "List is already empty\n");
        return;
    }

    // Save head
    temp = dev->head;
    
    // Move head to next node
    dev->head = dev->head->next;
    
    list_device.data_len -= strlen(temp->data) + 1;
    // Free node data and node
    kfree(temp->data);
    kfree(temp);

    dev->list_size--;
}
 
static void delete_all(struct linked_list_dev *dev) {
    while (dev->head) {
        delete_front(dev);
    }
}

// Device Open
static int dev_open(struct inode *inodep, struct file *filep) {
    // Optional: Add any necessary open logic
    return 0;
}
 
// Device Release
static int dev_release(struct inode *inodep, struct file *filep) {
    // Optional: Add any necessary close logic
    return 0;
}
 
 // Device Read
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    struct node *current_node;
    size_t bytes_to_copy = 0;
    size_t copied = 0;
    char *output_buffer;
 
    // Protect list access
    mutex_lock(&list_device.list_mutex);
    // Allocate memory for the output_buffer
    output_buffer = kmalloc((list_device.data_len + 1), GFP_KERNEL);
    if (!output_buffer) {
        mutex_unlock(&list_device.list_mutex);
        printk(KERN_ERR "Failed to allocate memory for the output buffer.\n");
        return -ENOMEM;
    } 

    // Check if we've reached end of list or list is empty
    if (!list_device.head || *offset >= list_device.data_len) {
        mutex_unlock(&list_device.list_mutex);
        return 0;
    }
 
    // Get the head node's data
    current_node = list_device.head;

    while (current_node) {
        // Copy data to temp buffer
        bytes_to_copy += snprintf(output_buffer + bytes_to_copy, list_device.data_len - bytes_to_copy + 1, "%s\n", current_node->data);
        current_node = current_node->next;
    }

    copied = min(bytes_to_copy - *offset, len);    
    
    if (copy_to_user(buffer, output_buffer + *offset, copied)) {
        kfree(output_buffer);
        mutex_unlock(&list_device.list_mutex);
        printk(KERN_ERR "Failed to copy buffer to user space.\n");
        return -EFAULT;
    
    }
    *offset += copied;
    kfree(output_buffer);
     
    mutex_unlock(&list_device.list_mutex);
 
    // // Return number of bytes successfully copied
    return copied;
}

// Device Write
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char input_buffer[MAX_INPUT_SIZE] = {0};
    char command[5] = {0};
    char data[MAX_INPUT_SIZE] = {0};
    int ret;

    // Ensure we don't exceed buffer size
    len = min(len, sizeof(input_buffer) - 1);

    // Copy from user space
    if (copy_from_user(input_buffer, buffer, len)) {
        return -EFAULT;
    }

    // Null-terminate the input
    input_buffer[len] = '\0';

    // Trim trailing newline
    if (input_buffer[len-1] == '\n') {
        input_buffer[len-1] = '\0';
    }

    // Protect list access
    mutex_lock(&list_device.list_mutex);

    // Parse input
    ret = sscanf(input_buffer, "%4s %255[^\n]", command, data);
    // Process commands
    if (strcmp(command, "ADDF") == 0 && ret == 2) {
        add_front(&list_device, data);
        printk(KERN_INFO "Added to front: %s\n", data);
    }
    else if (strcmp(command, "ADDB") == 0 && ret == 2) {
        add_back(&list_device, data);
        printk(KERN_INFO "Added to back: %s\n", data);
    }
    else if (strcmp(command, "DELF") == 0 && ret == 1) {
        delete_front(&list_device);
        printk(KERN_INFO "Deleted from front\n");
    }
    else if (strcmp(command, "DELA") == 0 && ret == 1) {
        delete_all(&list_device);
        printk(KERN_INFO "Deleted entire list\n");
    }
    else {
        mutex_unlock(&list_device.list_mutex);
        printk(KERN_WARNING "Invalid command or format\n");
        return -EINVAL;
    }

    mutex_unlock(&list_device.list_mutex);

    return len;
}

// Module Initialization
static int __init linked_list_init(void) {
    // Initialize the device structure
    list_device.head = NULL;
    list_device.list_size = 0;
    list_device.data_len = 0;
    mutex_init(&list_device.list_mutex);

    // Register major number
    list_device.major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (list_device.major_number < 0) {
        printk(KERN_ERR "Failed to register major number\n");
        return list_device.major_number;
    }

    // Create device class
    list_device.device_class = class_create(CLASS_NAME);
    if (IS_ERR(list_device.device_class)) {
        unregister_chrdev(list_device.major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create device class\n");
        return PTR_ERR(list_device.device_class);
    }

    // Create device
    list_device.device = device_create(
        list_device.device_class, 
        NULL, 
        MKDEV(list_device.major_number, 0), 
        NULL, 
        DEVICE_NAME
    );
    if (IS_ERR(list_device.device)) {
        class_destroy(list_device.device_class);
        unregister_chrdev(list_device.major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create device\n");
        return PTR_ERR(list_device.device);
    }

    printk(KERN_INFO "Linked List Device Registered\n");
    return 0;
}

// Module Cleanup
static void __exit linked_list_exit(void) {
    // Delete entire list
    delete_all(&list_device);

    // Destroy device
    device_destroy(list_device.device_class, MKDEV(list_device.major_number, 0));
    
    // Destroy class
    class_destroy(list_device.device_class);
    
    // Unregister major number
    unregister_chrdev(list_device.major_number, DEVICE_NAME);

    printk(KERN_INFO "Linked List Device Unregistered\n");
}

// Register initialization and exit functions
module_init(linked_list_init);
module_exit(linked_list_exit);