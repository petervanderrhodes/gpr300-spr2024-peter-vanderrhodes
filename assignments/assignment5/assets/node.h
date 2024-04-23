#include <vector>

#include <ew/transform.h>

struct Node {
	ew::Transform localTransform;
	ew::Transform globalTransform;
	Node* parent;
	std::vector<Node*> children;
	unsigned int numChildren;
	unsigned int parentIndex;
};


void SolveFKRecursive(Node* node);
void setNodeValues(Node newNode, glm::mat4 GlobalT, Node* parent = nullptr);