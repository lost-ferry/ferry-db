# Ferry DB
An embedded in-memory data-structure DB with proper type system.
Supports async background persistence.


## Available data structures
- Key Value Store
- Graph (Weighted)
- Multi-Graph (Weighted)
- State Machine
- Indexed Table

## Properties
| Property | Status         | Comments                                                                              |
| -------- |----------------|---------------------------------------------------------------------------------------|
| Atomic | n/a            | Only allows one operation at a time and either it is successful or unsuccessful.      |
| Consistency | Read Committed | A read returns only committed data.                                                   |
| Isolation | Read Committed | In the event of concurrent read and write only, reads can only retrieve last committed data. |
| Durability | Intermittent   | Operations occur in memory and sync to file system happens every second on a background thread. |
| Conflict Resolution | Last write wins | Transactions are ordered by timestamp of receipt and last write wins. |


## Use Case#1: Namespaced Key-Value Store
The developer can create namespaces and within each namespace they can create and query key 
to value mapping. Keys in separate namespaces cannot collide.

### Supported Types
- Text / Unicode
- Integer / 64-bit
- Float / 64-bit
- Single Bit Boolean
- (Advanced) Point in a 3D space
- (Advanced) 3D Topology

```
# Add a new key-value to the store
Set(Namespace, Key, Value)

# Fetch a value from the store
Get(Namespace, Key)

# Delete a value from the store
Delete(Namespace, Key)
```


## Use Case#2: Namespaced Graphs w/o Optional Weights
The developer can create and query graphs within a namespace with the following abilities:
- The nodes can carry arbitrary data
- Edges can have initially assignable weights
- The weights can be updated by the developer via the API
- A simple API to create an unweighted graph (internally, it is just all weights are 1 and never change.)

### Advanced use cases
- Weights can also be updated via traversal path taken to arrive at a node

```
# Create a node/vertex
AddNode(Namespace, Graph, Node, NodeData)

# Add an edge between two nodes with optional weight
AddEdge(Namespace, Graph, FromNode, ToNode, Weight)

# Update the weights between nodes
UpdateWeight(Namespace, Graph, FromNode, ToNode, NewWeight)

# Get out bound nodes
GetOutboundNodes(Namespace, Graph, FromNode)

# Get in bound nodes
GetInBoundNodes(Namespace, Graph, ToNode)

# Delete an Edge - should check for orphan graphs
DeleteEdge(Namespace, Graph, FromNode, ToNode)

# Delete a Node - should check for orphan graphs
DeleteNode(Namespace, Graph, Node)
```


# Use Case#3: Namespaced Graph Traversals
For graphs created in Use Case#2, we create a data structure which allows the 
developer to create and query traversals. Essentially, there is cursor which
contains the information about its current node and traversal history to that node.

```
# Create a traversal for Namespace/Graph with name, Traversal, at Node
AddTraversal(Namespace, Graph, Traversal, Node)

# Return the current node and the traverse history
GetTraversal(Namespace, Graph, Traversal)
```


# Use Case#4: Ability to define Namespaced Multi Graph with optional weights
Everything in use case two but two Nodes can have more than one edge between
them. Each edge between two Nodes will have a required name and that can has to be
unique between the two Nodes.
```
# Create a node/vertex
AddNode(Namespace, Graph, Node, NodeData)

# Add an edge between two nodes with optional weight
AddEdge(Namespace, Graph, EdgeName, FromNode, ToNode, Weight)

# Update the weights between nodes
UpdateWeight(Namespace, Graph, EdgeName, FromNode, ToNode, NewWeight)

# Get out bound nodes
GetOutboundNodes(Namespace, Graph, FromNode)

# Get in bound nodes
GetInBoundNodes(Namespace, Graph, ToNode)

# Delete an Edge - should check for orphan graphs
DeleteEdge(Namespace, Graph, FromNode, ToNode, EdgeName)

# Delete a Node - should check for orphan graphs
DeleteNode(Namespace, Graph, Node)
```


# Use Case#5: Namespaced Multi Graph Traversals
Same as Use Case#3 but for Multi Graphs: ability create and retrieve traversals.

```
# Create a traversal for Namespace/Graph with name, Traversal, at Node
AddTraversal(Namespace, Graph, Traversal, Node, EdgeName)

# Return the current node and the traverse history with the EdgeName
GetTraversal(Namespace, Graph, Traversal)
```


# Use Case#6: Namespaced State Machine
The developer can define a series states and transition rules.

```
# Create a state
AddState(Namespace, Machine, State, StateData)

# Add a transition
AddTransition(Namespace, Machine, Trigger, FromState, ToState)

# Remove a state
RemoveState(Namespace, Machine, State)

# Remove a transition
RemoveTransition(Namespace, Machine, Trigger, FromState, ToState)
```


# Use Case#7: State Machine Instances
Allow the developer to create instances of State Machine(s) defined in Use Case#6
and retrieve their current state and query the transition history.

```
# Create an instance of a, Machine, with the opening state, State.
CreateStateMachine(Namespace, Machine, Instance, State)

# Get an instance to retrieve the current state and transition history
GetStateMachine(Namespace, Machine, Instance)

# Trigger a transition
TriggerStateMachine(Namespace, Machine, Instance, Trigger)
```


# Use Case#8: Namespaced Indexed Table
Allow the developer to create a define a table structure with an index column.
The index column only has unique values. The table supports the following data
types:

Index column once created cannot be deleted or altered.

```
# Create a table by providing a name and the index column details
AddIndexedTable(Namespace, Table, IndexCol, IndexColType)

# Add a column
AddColumn(Namespace, Table, Col, ColType)

# Remove a column
RemoveColumn(Namespace, Table, Col)

# Insert a row
InsertRow(Namespace, Table, Index, ...Varargs)

# Update a row - Vaargs can be partial and only those rows can be affected
UpdateRow(Namespace, Table, Index, ...Varargs)

# Delete a row
DeleteRow(Namespace, Table, Index)

# Drop a table
DropIndexedTable(Namespace, Table)
```


# Use Case#9: (Advanced) Namespaced Cross Platform Async Firehose
All the above operation publish to a central pub-sub queue and provide an interface
to the developer to listen for events on this queue. The architecture is that of a 
firehose all events are published to the queue as received, and it is upon the consumers
to filter, buffer them among other things.

```
# Subscribe to one or more events or a wildcard like *
Subscribe(Namespace, Event)
```


# Use Case#10: (Advanced) Async file system persistent
The data is intermittently persisted to a file system in a non-blocking manner
which is cross-platform. There can be a potential data loss if the system crashes
after the last-sync but before the next sync.


# Use Case#11: (Advanced) Dump & Restore
Ability to dump to and restore from a flat file the entire database or a specific Namespace.


# Use Case#12: Command Line Interface for convenience
A simple cli-tool which exposes all the operations plus management commands like
backup and restores for convenience.
