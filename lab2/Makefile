MONGO_CONTAINER := robot-mongo
MONGO_PORT := 27017
MONGO_IMAGE := docker.io/library/mongo:latest
MONGO_DATA := mongo_data

CONFIG := config.yaml
PYTHON := python3

.PHONY: mongo-up mongo-down mongo-shell mongo-logs \
	mongo-up-brew mongo-down-brew mongo-install-brew mongo-shell-brew \
	install run clean

## Поднять MongoDB в Docker (данные хранятся в volume mongo_data)
mongo-up:
	docker run -d --name $(MONGO_CONTAINER) \
		-p $(MONGO_PORT):27017 \
		-v $(CURDIR)/$(MONGO_DATA):/data/db \
		$(MONGO_IMAGE)

## Остановить MongoDB (контейнер сохраняется, данные не теряются)
mongo-down:
	docker stop $(MONGO_CONTAINER) && docker rm $(MONGO_CONTAINER)

## Зайти в консоль MongoDB (mongosh)
mongo-shell:
	docker exec -it $(MONGO_CONTAINER) mongosh

## Логи MongoDB
mongo-logs:
	docker logs -f $(MONGO_CONTAINER)

## Установить MongoDB через Homebrew (один раз)
mongo-install-brew:
	brew tap mongodb/brew && brew install mongodb-community

## Поднять MongoDB через brew (сервис, автозапуск)
mongo-up-brew:
	brew services start mongodb-community

## Остановить MongoDB через brew
mongo-down-brew:
	brew services stop mongodb-community

## Зайти в консоль MongoDB (mongosh, brew-вариант)
mongo-shell-brew:
	mongosh

## Установить зависимости Python
install:
	$(PYTHON) -m pip install --user pyyaml pymongo requests

## Запустить поискового робота
run:
	$(PYTHON) robot.py $(CONFIG)

## Полная очистка: контейнер + данные MongoDB
clean:
	docker rm -f $(MONGO_CONTAINER) 2>/dev/null || true
	rm -rf $(MONGO_DATA)

help:
	@grep -E '^## ' $(MAKEFILE_LIST) | sed 's/## //'
