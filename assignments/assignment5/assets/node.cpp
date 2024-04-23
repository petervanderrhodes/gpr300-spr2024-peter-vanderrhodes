#include "node.h"


void SolveFKRecursive(Node* node)
{
	if (node->parent == nullptr)
	{
		node->globalTransform.modelMatrix() = node->localTransform.modelMatrix();
	}
	else {
		node->globalTransform.modelMatrix() = node->parent->globalTransform.modelMatrix() * node->localTransform.modelMatrix();
	}
	for(Node* child : node->children)
	{
		SolveFKRecursive(child);
	}
}

void setNodeValues(Node newNode, glm::mat4 GlobalT, Node* parent)
{
	newNode.globalTransform.modelMatrix() = GlobalT;
	newNode.localTransform.modelMatrix() = glm::mat4(0);
	newNode.parent = parent;
	if (newNode.parent == nullptr)
	{
		//break
		newNode.parentIndex = 1;
	}
}
