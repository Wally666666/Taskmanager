#ifndef LINKED_LIST_HPP
#define LINKED_LIST_HPP

#include "splashkit.h"

/**
 * A node is a class that contains a pointer to the next node,
 * and a data value.
 *
 * @tparam T The type of the data that will be stored in the node.
 * @field next A pointer to the next node in the list.
 * @field data The data that is stored in the node.
 */
template <typename T>
class node
{
public:
    T data;
    node *next;
};

/**
 * A linked list is a class that contains a pointer to the
 * first node, and the last node of the list.
 *
 * @tparam T The type of the data that will be stored in the list.
 * @field first A pointer to the first node in the list.
 * @field last A pointer to the last node in the list.
 */
template <typename T>
class linked_list
{
public:
    node<T> *first;
    node<T> *last;

    /**
     * Default constructor for linked_list.
     * Initializes an empty list with first and last set to nullptr.
     */
    linked_list()
    {
        first = nullptr;
        last = nullptr;
    }

    /**
     * Add a new node to the end of the list.
     *
     * @param data The data to store in the new node.
     * @return A pointer to the newly created node.
     */
    node<T> *add_node(T data)
    {
        node<T> *new_node = new node<T>();

        new_node->data = data;
        new_node->next = nullptr;

        if (first == nullptr)
        {
            first = new_node;
        }
        else
        {
            last->next = new_node;
        }

        last = new_node;

        return new_node;
    }

    /**
     * Finds and returns the node previous to `target_node`.
     *
     * @param target_node The target node to find the previous node for.
     * @return A pointer to the node before `target_node`.
     * @throws string If the target node is not found in the list.
     */
    node<T> *find_previous_node(node<T> *target_node)
    {
        if (target_node == first)
            return nullptr;

        node<T> *current = first;
        while (current != nullptr)
        {
            if (current->next == target_node)
            {
                return current;
            }

            // move to the next node
            current = current->next;
        }

        // If we couldn't find it...probably best to throw an exception
        throw string("find_previous_node search failed: node not in list.");
    }

    /**
     * Insert a new node before the specified target node.
     * @param target_node The target node to be inserted before it.
     * @param value The data to store in the new node.
     */
    void insert_before(node<T> *target_node, T value)
    {
        node<T> *new_node = new node<T>();
        new_node->data = value;

        // Insert at the head of the list
        if (target_node == first)
        {
            new_node->next = first;
            first = new_node;
            return;
        }

        // Find the previous node and insert between nodes
        node<T> *prev = find_previous_node(target_node);
        new_node->next = target_node;
        prev->next = new_node;
    }
    /**
     * Insert a new node after the specified target node.
     * @param target_node The target node to be inserted after it.
     * @param value The data to store in the new node.
     */
    void insert_after(node<T> *target_node, T value)
    {
        node<T> *new_node = new node<T>();

        new_node->data = value;
        new_node->next = target_node->next;
        target_node->next = new_node;

        // Insert at the last of the list
        if (target_node == last)
        {
            last = new_node;
        }
    }

    /**
     * Add a value to the start of the linked list.
     * @param value The data to store in the new node.
     *
     */
    void prepend(T value)
    {
        node<T> *new_node = new node<T>();

        new_node->data = value;
        new_node->next = first;
        first = new_node;

        // Insert at the last of the list
        if (last == nullptr)
        {
            last = new_node;
        }
    }
    /**
     * Remove the indicated node from the list.
     *
     * @param del_node The node to remove from the list.
     */
    void remove(node<T> *del_node)
    {
        node<T> *previous_node = nullptr;

        // Check if we are removing the first node.
        if (first == del_node)
        {
            // Set first to the 2nd node, if it exists
            first = del_node->next;
            // If it is the first node, then
            // previous_node is already the correct value.
            // I'll assign it anyway for clarity.
            previous_node = nullptr;
        }
        else
        {
            // Find the previous node
            previous_node = find_previous_node(del_node);

            // Make its connection skip over `del_node`
            previous_node->next = del_node->next;
        }

        // Similarly, check for removing the last node
        if (last == del_node)
        {
            last = previous_node;
        }

        // Delete the node from memory
        delete del_node;
    }

    /**
     * Clear a linked list by deleting all nodes.
     */
    void clear()
    {
        node<T> *current = first;
        while (current != nullptr)
        {
            // take a copy of the next node's pointer _before_ deleting current
            node<T> *next = current->next;

            delete current;

            current = next;
        }

        // Reset the first and last
        first = nullptr;
        last = nullptr;
    }

    /**
     * Destructor for linked_list.
     * Clears all nodes when the list is destroyed.
     */
    ~linked_list()
    {
        clear();
    }
};

#endif // LINKED_LIST_HPP
