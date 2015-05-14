# BatchSet
When std::set and std::unordered_set have too much overhead there's this. BatchSet was created because I needed to have a set of over 408 million, 64 bit integers and I don't have about 38 GiB of RAM.

## Overhead

### std::unordered_set
2 pointers (16 bytes/bucket)<br>
2 pointers and the data (24 bytes/element)<br>
OS overhead (56 bytes/allocation [ie per element])<br>
-----------<br>
101.03 bytes/element (using default load factor and 408 million elements)

### std::set
3 pointers, 2 chars, and the data (40 bytes/element)<br>
OS overhead (56 bytes/allocation [ie per element])<br>
-----------<br>
96 bytes/element

### BatchSet
Temp buffer size of 2^20 (8 MiB)<br>
8 bytes/element<br>
-----------<br>
8.02 bytes/element

You can over estimate the amount of unique elements if your OS does lazy allocation then this won't need to reallocate and copy which will cost twice the memory.

## Behind the Scenes
On insert, data is appended to the temp buffer until it is full then it's merged into the main array. All other operations besides clear() will first merge the temp buffer into the main array.

Merge starts by sorting the temp buffer then removing all duplicates in the union of the main array and temp buffer. This also finds the total unique elements which allows you to do a semi-inplace merge into the main array. By starting at the new last position in the main array and stepping in reverse through the main array and temp buffer.
