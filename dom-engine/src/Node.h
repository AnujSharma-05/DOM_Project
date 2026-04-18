#ifdef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <memory>
#include <map>

class Node : public std::enable_shared_from_this<Node> {
    std::string type;
    std::map<std::string, std::string> props;
    std::vector<std::shared_ptr<Node>> children;
    std::weak_ptr<Node> parent; //weak pointer so as to prevent the cycle between child and parent. Parent owns child and child owns parent. then the reference counter is never 0 because of this cycle. so now parent to child -> strong but child -> parent is weak non-owning observer


    Node(const std:: string& type); //constructor

    void addChild(std::shared_ptr<Node> child); //we pass shred pointer to enforce the ownership clarity
    
    void setProp(const std:: string& key, const std:: string& value);

    void print(int depth =0);

}


#endif