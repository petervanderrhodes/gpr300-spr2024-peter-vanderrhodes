#include "node.h"


void SolveFKRecursive(Node* node)
{
	if (node->parent == nullptr)
	{
		node->globalTransform = node->localTransform;
	}
	else {
		node->globalTransform = node->parent->globalTransform * node->localTransform;
	}
	for(Node* child : node->children)
	{
		SolveFKRecursive(child);
	}
}

void setNodeParent(Node* newNode, Node* parent)
{
	//newNode->globalTransform.modelMatrix() = GlobalT;
	newNode->localTransform = glm::mat4(1);
	newNode->parent = parent;
	if (parent != nullptr)
	{
		parent->children.push_back(newNode);
	}
}
