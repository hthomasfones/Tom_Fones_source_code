#Linked List
#Insert at beginning
    def InsertHead(self, insnode):
        insnode.next = self.next
        self = insnode
        return

    def InsertAfter(self, insnode):
        insnode.next = self.next
        self.next = insnode
        return

    def FindListEnd(self):
        while (self.next is not None):
            self = self.next
        return self

    def AppendToList(self, insnode):
        if (self.next is not None):
            print("AppendToList invalid data!\n")
            return
        self.next = insnode
        insnode.next = None
        return

class Node:
   def __init__(self, dataval=None):
      self.dataval = dataval
      self.next = None

class FLinkedList:
    def __init__(self):
        self.head = None



list1 = FLinkedList()
list1.headval = Node(0)
N1 = Node(1)
N2 = Node(2)
N1.next = N2
list1.headval.next = N1

#Insert at beginning
    def InsertHead(self, insnode):
        #InsNode = Node(insdata)
        insnode.next = self.next
        self = insnode

#Insert at end
    def InsertTail(self, insnode):
        #nsNode = Node(insdata)
        if (self.headval is None):
            self.headval = InsNode
            return
        else:
            endnode = self.headval
            while (endnode.next is not None):
                endnode = endnode.next
            endnode.next = InsNode

#Insert after
    def Insert(self,targetnode, insdata):
        InsNode = Node(insdata)
        InsNode.next = targetnode.next
        targetnode.next = InsNode

#Remove node by keyval
    def RemoveNode(self, rkey):
        if (self.headval == None):
            return

        head = self.head
        if (head.data == rkey):
            self.head = head.next
            head = None
            return

        while (head is not None):
            if (head.data == rkey): break

            prev = head
            headval = head.next

        if (head is None): return
        else:
            prev.next = head.next
            head = None




#################################################################################################################
# Binary Tree
class Node:
   def __init__(self, data):
      self.left = None
      self.right = None
      self.data = data
#Insert node
   def insert(self, data):
   # Compare the new value with the parent node
       if (self.data is None): self.data = data
       else:
           if (data < self.data):
               if (self.left is None): self.left = Node(data)
               else: self.left.insert(data)
           else:
               if (data > self.data):
                   if self.right is None: self.right = Node(data)
                   else: self.right.insert(data)

   # Print the tree - recursive
   #def PrintTree(self):
# Inorder traversal
# Left -> Root -> Right
   def inorderTraversal(self, root):
      res = []
      if root:
         res = self.inorderTraversal(root.left)
         res.append(root.data)
         res = res + self.inorderTraversal(root.right)
      return res

#####################################################################
#Binary Search Tree
# findval method to compare the value with nodes
   def findval(self, seekval):
      if (seekval < self.data):
         if self.left is None: return False
         else: return self.left.findval(seekval)
      else:
          if (seekval > self.data):
              if (self.right is None): return False
              else: return self.right.findval(seekval)
          else: return True

#########################################################
#Heap tree
import heapq
HR = [21,1,45,78,3,5]
heapq.heapify(HR)

#Add to heap
heapq.heappush(HR,8)

# Remove element from the heap - 1st node
heapq.heappop(HR)

# Replace an element - remove 1st node, insert node random
heapq.heapreplace(HR,6)

############################################################
# Definition for singly-linked list.
# class ListNode(object):
#     def __init__(self, val=0, next=None):
#         self.val = val
#         self.next = next

def Insert(self, insdata):
    # print(insdata, "\n")
    insnode = Node(insdata)
    while (self is not None):
        if (insdata >= self.val):
            if (insdata <= self.next.val): break
        self = self.next

    insnode.next = self.next
    self.next = insnode
    # print(self, "\n")


class Node:
    def __init__(self, dataval=0, next=None):
        self.val = dataval
        self.next = None


#
class FLinkedList:
    def __init__(self):
        self.head = None


# Insert at end
def InsertTail(self, insnode):
    # print(insnode.val, "\n")
    if (self.head is None):
        self.head = insnode
        return
    else:
        endnode = self.head
        while (endnode.next is not None):
            endnode = endnode.next
        endnode.next = insnode


class Solution(object):
    def mergeTwoLists(self, list1, list2):
        """
        :type list1: Optional[ListNode]
        :type list2: Optional[ListNode]
        :rtype: Optional[ListNode]
        """
        print(list1, "\n")
        print(list2, "\n")

        if ((list1 is None) and (list2 is None)): return None
        if (list1 is None): return list2
        if (list2 is None): return list1

        rlist = list1
        if (list2 is None): return rlist

        for x in range(0, 10000):
            if (list2 is None): break

            Insert(rlist, list2.val)
            list2 = list2.next

        print(rlist, "\n")
        return rlist
