#include <random>

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

struct Node
{
	float x;
	float y;

	Node() {}
	Node(float _x, float _y) : x(_x), y(_y) {}

	bool operator == (const Node& rhs) const
	{
		if (rhs.x == this->x && rhs.y == this->y)
		{
			return true;
		}

		return false;
	}

	friend std::ostream& operator << (std::ostream& os, const Node* n)
	{
		os << '(' << n->x << ", " << n->y << ')';
		return os;
	}

	~Node()
	{
		delete &x;
		delete &y;
	}
};

// Represents any N-gon in 2d
class Poly2d
{
public:
	std::vector<Node*> vertices;

public:
	void bestRoute(olc::PixelGameEngine* pge)
	{
		int len = vertices.size();
		for (int i = 0; i < len; i++)
		{
			pge->DrawLine(vertices[i]->x, vertices[i]->y, vertices[(i + 1) % len]->x, vertices[(i + 1) % len]->y, olc::WHITE);
		}
	}

	virtual void drawSelf(olc::PixelGameEngine* pge) = 0;
};

// 12-gon
class Dodecagon : public Poly2d
{
public:
	Dodecagon()
	{
		float pi = 3.1415926f;
		float theta = 0.0f;
		float d = 40.0f / std::sinf(pi / 12.0f); // distance from origin to vertex

		// Fill in the vertices
		for (int i = 0; i < 12; i++)
		{
			vertices.push_back(new Node{ 200.0f + d * std::sinf(theta), 250.0f + d * std::cosf(theta) });
			theta += (2 * pi / 12.0f);
		}
	}

	// Draws only the vertices of the polygon
	void drawSelf(olc::PixelGameEngine* pge) override
	{
		for (auto& v : vertices)
		{
			pge->DrawRect(v->x, v->y, 1, 1, olc::RED);
		}
	}
};

class Heptagon : public Poly2d
{
public:
	Heptagon()
	{
		float pi = 3.1415926f;
		float theta = 0.0f;
		float d = 40.0f / std::sinf(pi / 7.0f); // distance from origin to vertex

		// Fill in the vertices
		for (int i = 0; i < 7; i++)
		{
			vertices.push_back(new Node{ 200.0f + d * std::sinf(theta), 250.0f + d * std::cosf(theta) });
			theta += (2 * pi / 7.0f);
		}
	}

	// Draws only the vertices of the polygon
	void drawSelf(olc::PixelGameEngine* pge) override
	{
		for (auto& v : vertices)
		{
			pge->DrawRect(v->x, v->y, 1, 1, olc::RED);
		}
	}
};

class Icosagon : public Poly2d
{
public:
	Icosagon()
	{
		float pi = 3.1415926f;
		float theta = 0.0f;
		float d = 20.0f / std::sinf(pi / 20.0f); // distance from origin to vertex

		// Fill in the vertices
		for (int i = 0; i < 20; i++)
		{
			vertices.push_back(new Node{ 200.0f + d * std::sinf(theta), 250.0f + d * std::cosf(theta) });
			theta += (2 * pi / 20.0f);
		}
	}

	// Draws only the vertices of the polygon
	void drawSelf(olc::PixelGameEngine* pge) override
	{
		for (auto& v : vertices)
		{
			pge->FillCircle(v->x, v->y, 2, olc::RED);
		}
	}
};

class Route
{
public:
	std::vector<Node*> nodes;

	Route() {}

	// Returns a measure of how good a route is (the greater the output the better)
	float fitness()
	{
		auto distance = [](const Node* n1, const Node* n2)
		{
			return sqrtf((n2->x - n1->x) * (n2->x - n1->x) + (n2->y - n1->y) * (n2->y - n1->y));
		};

		float totDist = 0.0f;
		for (int i = 0; i < nodes.size(); i++)
		{
			totDist += distance(nodes[i], nodes[(i + 1) % nodes.size()]);
		}

		return 1.0f / totDist;
	}

	// Draw the lines between consecutive nodes
	void displayRoute(olc::PixelGameEngine* pge)
	{
		for (int i = 0; i < nodes.size(); i++)
		{
			pge->DrawLine(nodes[i]->x, nodes[i]->y, nodes[(i + 1) % nodes.size()]->x, nodes[(i + 1) % nodes.size()]->y, olc::WHITE);
		}
	}

	// For debug purposes
	void printRoute()
	{
		for (auto* n : nodes)
		{
			std::cout << n << "->";
		}
		std::cout << std::endl;
	}
};

class Generation
{
public:
	std::vector<Route> population;

	Generation() {}
};

class GeneticAlgorithm : public olc::PixelGameEngine
{
public:
	GeneticAlgorithm()
	{
		sAppName = "Genetic Algorithm on Polygon Visualization";
	}

private:
	int iteration;

	Dodecagon testPoly;

	int popSize;
	Generation currGen;
	Generation nextGen;

	std::pair<Route, Route> currParents;
	Route currChild;

	float crossoverChance;
	float mutationChance;

public:
	// Selects two random "parents" from the current generation, higher fitness -> higher probability of selection
	void selection(Generation& gen, std::pair<Route, Route>& parents)
	{
		bool foundFirstParent = false;
		bool foundSecondParent = false;

		while (!foundFirstParent)
		{
			for (auto& route : gen.population)
			{
				float p = route.fitness();
				if (((float)rand() / (float)RAND_MAX) < p)
				{
					parents.first = route;
					foundFirstParent = true;
				}
			}
		}

		while (!foundSecondParent) 
		{
			for (auto& route : gen.population)
			{
				const float p = route.fitness();
				if ((float)rand() / (float)RAND_MAX < p && route.nodes != parents.first.nodes)
				{
					parents.second = route;
					foundSecondParent = true;
				}
			}
		}
	}

	// 2 parents may create a child
	void crossover(const std::pair<Route, Route>& parents, Route& child)
	{
		if (((float)rand() / (float)RAND_MAX) < crossoverChance)
		{
			// Choose a parent and pass down consecutive genes of random length
			const int n = testPoly.vertices.size();
			child.nodes.resize(n);
			
			const int rand1 = rand() % n;
			const int rand2 = (rand1 + rand() % n) % n;

			int loindex = std::min(rand1, rand2);
			int hiindex = std::max(rand1, rand2);
			
			for (int i = loindex; i <= hiindex; i++)
			{
				child.nodes[i] = parents.first.nodes[i];
			}

			// Fill the child with genes from other parent
			int childId = (hiindex + 1) % n;
			int parentId = (hiindex + 1) % n;
			int missing = n - (hiindex - loindex + 1);

			while (missing > 0)
			{
				if (std::find(child.nodes.begin(), child.nodes.end(), parents.second.nodes[parentId]) != child.nodes.end())
				{
					// Gene is already in child
					parentId = (parentId + 1) % n;
					
				}
				else
				{
					child.nodes[childId] = parents.second.nodes[parentId];
					childId = (childId + 1) % n;
					parentId = (parentId + 1) % n;
					missing--;
				}
			}
		}
		else
		{
			child = parents.first;
		}
	}

	// A child may go through mutation
	void mutate(Route& child)
	{
		if (((float)rand() / (float)RAND_MAX) < mutationChance)
		{
			const int len = testPoly.vertices.size();

			// Switch two randomly chosen genes
			int baseIndex = rand() % len;
			int otherIndex = (baseIndex + rand() % len) % len;

			// Perform swap
			std::swap(child.nodes[baseIndex], child.nodes[otherIndex]);
		}
	}

public:
	bool OnUserCreate() override
	{
		iteration = 0;

		popSize = 100; // population size
		crossoverChance = 0.30f;
		mutationChance = 0.05f;

		// Generation 0
		srand(time(NULL));
		for (int i = 0; i < popSize; i++)
		{
			// Create copy of shape vertices
			Route shuffledPolyNodes;
			shuffledPolyNodes.nodes = testPoly.vertices;

			// Random shuffle
			std::shuffle(std::begin(shuffledPolyNodes.nodes), std::end(shuffledPolyNodes.nodes), std::default_random_engine(rand()));

			currGen.population.push_back(shuffledPolyNodes);
		}

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// Clear Screen
		Clear(olc::VERY_DARK_GREY);

		testPoly.drawSelf(this);

		if (GetKey(olc::Key::P).bHeld || GetKey(olc::Key::S).bReleased)
		{
			++iteration;
			while (nextGen.population.size() != currGen.population.size())
			{
				// Select 2 parents
				selection(currGen, currParents);

				// Create child
				crossover(currParents, currChild);
				
				// Mutate child
				mutate(currChild);

				// Add to new generation
				nextGen.population.push_back(currChild);

				// Reset for next iteration
				currParents.first.nodes.clear();
				currParents.second.nodes.clear();
				currChild.nodes.clear();
			}

			// Next gen becomes current gen
			currGen = nextGen;

			nextGen.population.clear();
		}

		// View the first route of current generation
		currGen.population[0].displayRoute(this);
		
		// Display Controls
		DrawString(100, 10, "(S): Step", olc::WHITE);
		DrawString(100, 20, "(P): Play", olc::WHITE);

		// Display statistics
		DrawString(200, 10, "First route: " + std::to_string(1.0f / currGen.population[0].fitness()), olc::WHITE);
		DrawString(200, 20, "Population size: " + std::to_string(popSize), olc::WHITE);
		DrawString(200, 30, "Crossover Chance: " + std::to_string(crossoverChance), olc::WHITE);
		DrawString(200, 40, "Mutation Chance: " + std::to_string(mutationChance), olc::WHITE);
		DrawString(200, 50, "# Iterations: " + std::to_string(iteration), olc::WHITE);

		return true;
	}
};

int main()
{
	GeneticAlgorithm demo;
	if (demo.Construct(480, 480, 2, 2))
	{
		demo.Start();
	}

	return 0;
}