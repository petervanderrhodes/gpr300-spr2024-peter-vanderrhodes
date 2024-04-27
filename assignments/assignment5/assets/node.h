#include <vector>

#include <ew/transform.h>

struct Node {
	glm::mat4 localTransform;
	glm::mat4 globalTransform;
	Node* parent;
	std::vector<Node*> children;
	//unsigned int numChildren;
	//unsigned int parentIndex;
	bool hasModel = true;
};


void SolveFKRecursive(Node* node);
void setNodeParent(Node* newNode, Node* parent = nullptr);