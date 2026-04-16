import random
import config


class Connection:
    def __init__(self, from_node, to_node, weight):
        self.from_node = from_node
        self.to_node = to_node
        self.weight = weight

    def mutate_weight(self):
        # Higher chance for completely new weights in early generations
        if hasattr(config, 'generation'):
            # Adjust mutation rate based on generation
            mutation_chance = max(0.05, 0.3 - (config.generation * 0.01))
        else:
            mutation_chance = 0.1

        if random.uniform(0, 1) < mutation_chance:
            # Complete random weight reset
            self.weight = random.uniform(-1, 1)
        else:
            # Fine tuning - smaller adjustments as generations progress
            variance = 1.0
            if hasattr(config, 'generation'):
                variance = max(0.1, 1.0 - (config.generation * 0.02))

            self.weight += random.gauss(0, variance) / 5

            # Constrain weights
            if self.weight > 1:
                self.weight = 1
            if self.weight < -1:
                self.weight = -1

    def clone(self, from_node, to_node):
        clone = Connection(from_node, to_node, self.weight)
        return clone