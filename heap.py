#!/usr/bin/python3
class Heap(object):
    def __init__(self, data:list = []):
        self.the_heap = data

    def get_parent_index(self, index):
        return int(index / 2)

    def insert(self, num:int):
        self.the_heap.append(num)
        curr_index = len(self.the_heap) -1
        parent = curr_index

        while parent!= 0 :
            parent = self.get_parent_index(curr_index)
            if (self.the_heap[parent]  >  self.the_heap[curr_index]):
                # wap the parent and me 
                self.the_heap[curr_index] = self.the_heap[parent]
                self.the_heap[parent] = num
                curr_index = parent
            else:
                break
    def swap(self, i1:int, i2:int):
        tmp = self.the_heap[i1] 
        self.the_heap[i1] = self.the_heap[i2]
        self.the_heap[i2] = tmp
        
    def pop(self):
        # prepare for return 
        ret = self.the_heap[0]
        self.the_heap[0] = self.the_heap[-1]
        del self.the_heap[-1]

        # reordering the tree 
        curr = 0
        size = len(self.the_heap)

        while curr < size and curr*2  < size:
            succ_min_index = curr * 2
            if succ_min_index + 1< size and \
                self.the_heap[succ_min_index ] > self.the_heap[succ_min_index + 1]:
                succ_min_index += 1 
            if self.the_heap[curr] > self.the_heap[succ_min_index]:
                self.swap(curr, succ_min_index)
                curr = succ_min_index
            else: 
                break
        return ret

    def __str__(self):
        return str(self.the_heap)

    def dump_check(self):
        h = Heap(self.the_heap)
        arr = []
        for i in range(len(self.the_heap)):
            arr.append(h.pop()) 
        return arr

if __name__ == "__main__":
    import random
    h = Heap()
    

    print(h)
    for i in range(10):
        h.insert(random.randint(1,100))


    print(h)
    print(h.dump_check())