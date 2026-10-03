import yaml
import os
import sys
import time
import json
import hashlib
import logging
from datetime import datetime, timedelta
from urllib.parse import urlparse, urljoin, quote_plus
from typing import Dict, List, Optional, Any
import pymongo
from pymongo import MongoClient, UpdateOne
from bson import ObjectId
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


class MusicCrawler:
    """Поисковый робот для обкачки музыкальных документов"""
    
    def __init__(self, config_path: str):
        """
        Инициализация краулера
        
        Args:
            config_path: путь к YAML конфигурационному файлу
        """
        
        self.config = self.load_config(config_path)
        
        
        self.setup_logging()
        
        
        self.db = self.connect_to_mongodb()
        self.collection = self.db[self.config['db']['collection']]
        
        
        self.create_indexes()
        
        
        self.session = self.create_session()
        
        
        self.stats = {
            'total_crawled': 0,
            'new_documents': 0,
            'updated_documents': 0,
            'skipped_documents': 0,
            'failed_documents': 0,
            'start_time': time.time(),
            'last_checkpoint': None
        }
        
        
        self.running = True
        self.last_url = None
        
        self.logger.info("Поисковый робот инициализирован")
        self.logger.info(f"Источники: {list(self.config['sources'].keys())}")
        
    def load_config(self, config_path: str) -> Dict:
        """Загрузка YAML конфигурации"""
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                config = yaml.safe_load(f)
            
            
            required_sections = ['db', 'logic', 'sources']
            for section in required_sections:
                if section not in config:
                    raise ValueError(f"Отсутствует обязательная секция: {section}")
            
            return config
            
        except FileNotFoundError:
            print(f"Ошибка: Конфигурационный файл не найден: {config_path}")
            sys.exit(1)
        except yaml.YAMLError as e:
            print(f"Ошибка парсинга YAML: {e}")
            sys.exit(1)
        except Exception as e:
            print(f"Ошибка загрузки конфигурации: {e}")
            sys.exit(1)
    
    def setup_logging(self):
        """Настройка логирования"""
        log_config = self.config.get('logging', {})
        
        
        self.logger = logging.getLogger('MusicCrawler')
        self.logger.setLevel(getattr(logging, log_config.get('level', 'INFO')))
        
        
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        
        console_handler = logging.StreamHandler(sys.stderr)
        console_handler.setFormatter(formatter)
        self.logger.addHandler(console_handler)
        
        
        log_file = log_config.get('file', 'crawler.log')
        if log_file:
            file_handler = logging.FileHandler(log_file, encoding='utf-8')
            file_handler.setFormatter(formatter)
            self.logger.addHandler(file_handler)
    
    def connect_to_mongodb(self) -> MongoClient:
        """Подключение к MongoDB"""
        db_config = self.config['db']
        
        try:
            
            connection_string = f"mongodb://{db_config['host']}:{db_config['port']}"
            
            
            client = MongoClient(
                connection_string,
                serverSelectionTimeoutMS=5000
            )
            
            
            client.server_info()
            
            self.logger.info(f"Успешное подключение к MongoDB: {db_config['database']}")
            return client[db_config['database']]
            
        except Exception as e:
            self.logger.error(f"Ошибка подключения к MongoDB: {e}")
            sys.exit(1)
    
    def create_indexes(self):
        """Создание индексов для оптимальной работы"""
        try:
            
            self.collection.create_index([("url", pymongo.ASCENDING)], unique=True)
            
            
            self.collection.create_index([
                ("source", pymongo.ASCENDING),
                ("crawl_timestamp", pymongo.DESCENDING)
            ])
            
            
            self.collection.create_index([("content_hash", pymongo.ASCENDING)])
            
            self.logger.info("Индексы созданы успешно")
            
        except Exception as e:
            self.logger.warning(f"Ошибка создания индексов: {e}")
    
    def create_session(self) -> requests.Session:
        """Создание сессии requests с ретраями"""
        session = requests.Session()
        
        
        session.headers.update({
            'User-Agent': self.config['robots_txt'].get('user_agent', 'MusicCrawlerBot/1.0'),
            'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
            'Accept-Language': 'en-US,en;q=0.9',
            'Accept-Encoding': 'gzip, deflate',
            'Connection': 'keep-alive'
        })
        
        
        retry_strategy = Retry(
            total=self.config['logic'].get('retry_attempts', 3),
            backoff_factor=1,
            status_forcelist=[429, 500, 502, 503, 504],
            allowed_methods=["GET", "HEAD"]
        )
        
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session.mount("http://", adapter)
        session.mount("https://", adapter)
        
        return session
    
    def normalize_url(self, url: str, source_name: str) -> str:
        """
        Нормализация URL
        
        Args:
            url: исходный URL
            source_name: имя источника
            
        Returns:
            нормализованный URL
        """
        try:
            parsed = urlparse(url)
            
            
            if source_name == "Lyrics.ovh":
                
                return url
            elif source_name == "MusicBrainz":
                
                base_url = f"{parsed.scheme}://{parsed.netloc}{parsed.path}"
                return base_url
            
            
            normalized = f"{parsed.scheme}://{parsed.netloc}{parsed.path}"
            if parsed.query:
                normalized += f"?{parsed.query}"
            
            return normalized
            
        except Exception as e:
            self.logger.warning(f"Ошибка нормализации URL {url}: {e}")
            return url
    
    def calculate_content_hash(self, content: str) -> str:
        """Вычисление хэша контента для определения изменений"""
        return hashlib.md5(content.encode('utf-8')).hexdigest()
    
    def should_recrawl(self, document: Dict) -> bool:
        """
        Проверка, нужно ли переобкачивать документ
        
        Args:
            document: документ из базы данных
            
        Returns:
            True если нужно переобкачать
        """
        if not document:
            return True
        
        
        last_crawl = document.get('crawl_timestamp', 0)
        
        
        days_passed = (time.time() - last_crawl) / (24 * 3600)
        
        
        refresh_days = self.config['logic'].get('refresh_interval_days', 7)
        
        return days_passed > refresh_days
    
    def save_checkpoint(self, url: str):
        """Сохранение контрольной точки (последний обработанный URL)"""
        try:
            checkpoint_collection = self.db['crawler_checkpoints']
            
            checkpoint_data = {
                'crawler_id': 'music_crawler',
                'last_url': url,
                'timestamp': time.time(),
                'stats': self.stats
            }
            
            checkpoint_collection.update_one(
                {'crawler_id': 'music_crawler'},
                {'$set': checkpoint_data},
                upsert=True
            )
            
            self.last_url = url
            self.logger.debug(f"Контрольная точка сохранена: {url}")
            
        except Exception as e:
            self.logger.error(f"Ошибка сохранения контрольной точки: {e}")
    
    def load_checkpoint(self) -> Optional[str]:
        """Загрузка последней контрольной точки"""
        try:
            checkpoint_collection = self.db['crawler_checkpoints']
            
            checkpoint = checkpoint_collection.find_one(
                {'crawler_id': 'music_crawler'}
            )
            
            if checkpoint:
                self.last_url = checkpoint.get('last_url')
                self.logger.info(f"Загружена контрольная точка: {self.last_url}")
                return self.last_url
            
            return None
            
        except Exception as e:
            self.logger.error(f"Ошибка загрузки контрольной точки: {e}")
            return None
    
    def crawl_url(self, url: str, source_name: str, respect_robots_txt: bool = True) -> Optional[Dict]:
        """
        Обкачка одного URL
        
        Args:
            url: URL для обкачки
            source_name: имя источника
            
        Returns:
            словарь с данными документа или None при ошибке
        """
        try:
            
            normalized_url = self.normalize_url(url, source_name)
            
            self.logger.info(f"Обкачка: {normalized_url}")
            
            if respect_robots_txt:
                if not self.check_robots_txt(url, source_name):
                    self.logger.warning(f"URL заблокирован robots.txt: {url}")
                    return None
            
            
            response = self.session.get(
                url,
                timeout=self.config['logic'].get('timeout_seconds', 30)
            )
            
            response.raise_for_status()
            
            
            content = response.text
            
            content_type = response.headers.get('Content-Type', '')
            if 'text/html' not in content_type and 'json' not in content_type:
                self.logger.warning(f"Неподдерживаемый контент ({content_type}): {url}")
                return None
            
            
            content_hash = self.calculate_content_hash(content)
            
            
            title = self.extract_title(content, source_name)
            
            
            document = {
                'url': normalized_url,
                'original_url': url,
                'html_content': content,
                'content_hash': content_hash,
                'title': title,
                'source': source_name,
                'crawl_timestamp': int(time.time()),
                'response_code': response.status_code,
                'content_length': len(content),
                'content_type': response.headers.get('Content-Type', ''),
                'last_modified': response.headers.get('Last-Modified', '')
            }
            
            return document
            
        except requests.exceptions.Timeout:
            self.logger.error(f"Таймаут при обкачке: {url}")
            self.stats['failed_documents'] += 1
            return None
        except requests.exceptions.HTTPError as e:
            self.logger.error(f"HTTP ошибка {e.response.status_code}: {url}")
            self.stats['failed_documents'] += 1
            return None
        except Exception as e:
            self.logger.error(f"Ошибка обкачки {url}: {e}")
            self.stats['failed_documents'] += 1
            return None
    
    def check_robots_txt(self, url: str, source_name: str) -> bool:
        """Проверка robots.txt"""
        try:
            parsed = urlparse(url)
            robots_url = f"{parsed.scheme}://{parsed.netloc}/robots.txt"
            
            response = self.session.get(robots_url, timeout=5)
            
            if response.status_code == 200:
                
                user_agent = self.config['robots_txt'].get('user_agent', 'MusicCrawlerBot')
                robots_content = response.text
                
                
                lines = robots_content.split('\n')
                in_our_block = False
                
                for line in lines:
                    line = line.strip()
                    
                    if line.lower().startswith('user-agent:'):
                        agent = line[11:].strip()
                        in_our_block = (agent == '*' or user_agent in agent)
                    
                    elif in_our_block and line.lower().startswith('disallow:'):
                        path = line[9:].strip()
                        if path == '/' or parsed.path.startswith(path):
                            return False
                
            return True
            
        except Exception:
            
            return True
    
    def extract_title(self, html_content: str, source_name: str) -> str:
        """Извлечение заголовка из HTML"""
        try:
            
            if '<title>' in html_content and '</title>' in html_content:
                start = html_content.find('<title>') + 7
                end = html_content.find('</title>', start)
                title = html_content[start:end].strip()
                return title[:200]  
            
            
            if source_name == "Lyrics.ovh":
                
                if '<h1>' in html_content and '</h1>' in html_content:
                    start = html_content.find('<h1>') + 4
                    end = html_content.find('</h1>', start)
                    title = html_content[start:end].strip()
                    return title[:200]
            
            
            elif source_name == "MusicBrainz":
                if 'class="title"' in html_content:
                    
                    start = html_content.find('class="title"')
                    if start != -1:
                        start = html_content.find('>', start) + 1
                        end = html_content.find('<', start)
                        title = html_content[start:end].strip()
                        return title[:200]
            
            return f"Документ из {source_name}"
            
        except Exception:
            return f"Документ из {source_name}"
    
    def get_urls_from_source(self, source_name: str, source_config: Dict) -> List[str]:
        """
        Получение списка URL для обкачки из источника
        
        Args:
            source_name: имя источника
            source_config: конфигурация источника
            
        Returns:
            список URL для обкачки
        """
        urls = []
        
        if source_name == "lyrics_ovh":
            urls = self.get_lyrics_ovh_urls(source_config)
        elif source_name == "musicbrainz":
            urls = self.get_musicbrainz_urls(source_config)
        
        return urls
    
    def get_lyrics_ovh_urls(self, source_config: Dict) -> List[str]:
        """Генерация URL для Lyrics.ovh"""
        urls = []
        
        try:
            
            metadata_dir = source_config.get('metadata_dir', 'lyrics_corpus/metadata')
            
            if os.path.exists(metadata_dir):
                
                for filename in os.listdir(metadata_dir):
                    if filename.endswith('.json') and 'full' in filename:
                        filepath = os.path.join(metadata_dir, filename)
                        
                        try:
                            with open(filepath, 'r', encoding='utf-8') as f:
                                metadata = json.load(f)
                            
                            
                            artist = metadata.get('artist', '')
                            title = metadata.get('title', '')
                            
                            if artist and title:
                                    
                                    encoded_artist = quote_plus(artist)
                                    encoded_title = quote_plus(title)
                                    url = f"https://api.lyrics.ovh/v1/{encoded_artist}/{encoded_title}"
                                    urls.append(url)
                            
                        except Exception as e:
                            self.logger.warning(f"Ошибка чтения {filepath}: {e}")
            
            
            urls = list(dict.fromkeys(urls))
            
            if not urls:
                popular_tracks = [
                    ("The Beatles", "Yesterday"),
                    ("Queen", "Bohemian Rhapsody"),
                    ("Michael Jackson", "Billie Jean"),
                    ("Nirvana", "Smells Like Teen Spirit"),
                    ("Led Zeppelin", "Stairway to Heaven"),
                ]
                
                for artist, title in popular_tracks:
                    encoded_artist = quote_plus(artist)
                    encoded_title = quote_plus(title)
                    url = f"https://api.lyrics.ovh/v1/{encoded_artist}/{encoded_title}"
                    urls.append(url)
            
            self.logger.info(f"Сгенерировано {len(urls)} URL для Lyrics.ovh")
            
        except Exception as e:
            self.logger.error(f"Ошибка генерации URL для Lyrics.ovh: {e}")
        
        return urls
    
    def get_musicbrainz_urls(self, source_config: Dict) -> List[str]:
        """Генерация URL для MusicBrainz"""
        urls = []
        
        try:
            
            metadata_dir = source_config.get('metadata_dir', 'musicbrainz_corpus/metadata')
            
            if os.path.exists(metadata_dir):
                
                for filename in os.listdir(metadata_dir):
                    if filename.endswith('.json') and 'recording' in filename:
                        filepath = os.path.join(metadata_dir, filename)
                        
                        try:
                            with open(filepath, 'r', encoding='utf-8') as f:
                                metadata = json.load(f)
                            
                            
                            recording_id = metadata.get('mbid', '')
                            if recording_id:
                                
                                url = f"https://musicbrainz.org/ws/2/recording/{recording_id}?fmt=json"
                                urls.append(url)
                            
                        except Exception as e:
                            self.logger.warning(f"Ошибка чтения {filepath}: {e}")
            
            
            urls = list(dict.fromkeys(urls))
            
            if not urls:
                popular_recordings = [
                    "b3fbc2f9-7434-4d21-8bb3-1d246f5aef45",  
                    "d8f1f6bc-8e5c-42e5-9f5d-9b5f5b5f5b5f",  
                    "f6a3c7d8-9e2b-4f5d-8c3a-1b2e4f6a8c9d",  
                    "d4f1f6bc-8e5c-42e5-9f5d-9b5f5b5f5b5c",  
                    "c9f1f6bc-8e5c-42e5-9f5d-9b5f5b5f5b59",  
                ]
                
                for recording_id in popular_recordings:
                    url = f"https://musicbrainz.org/ws/2/recording/{recording_id}?fmt=json"
                    urls.append(url)
            
            self.logger.info(f"Сгенерировано {len(urls)} URL для MusicBrainz")
            
        except Exception as e:
            self.logger.error(f"Ошибка генерации URL для MusicBrainz: {e}")
        
        return urls
    
    def process_document(self, document: Dict) -> bool:
        """
        Обработка и сохранение документа в базу данных
        
        Args:
            document: документ для сохранения
            
        Returns:
            True если успешно сохранено
        """
        try:
            
            existing = self.collection.find_one({'url': document['url']})
            
            if existing:
                
                if existing.get('content_hash') == document['content_hash']:
                    
                    self.collection.update_one(
                        {'_id': existing['_id']},
                        {'$set': {'crawl_timestamp': document['crawl_timestamp']}}
                    )
                    self.stats['skipped_documents'] += 1
                    self.logger.debug(f"Документ не изменился: {document['url']}")
                    return True
                else:
                    
                    document['previous_versions'] = existing.get('previous_versions', []) + [{
                        'content_hash': existing.get('content_hash'),
                        'crawl_timestamp': existing.get('crawl_timestamp'),
                        'title': existing.get('title')
                    }]
                    
                    self.collection.update_one(
                        {'_id': existing['_id']},
                        {'$set': document}
                    )
                    self.stats['updated_documents'] += 1
                    self.logger.info(f"Документ обновлен: {document['url']}")
                    return True
            else:
                
                self.collection.insert_one(document)
                self.stats['new_documents'] += 1
                self.logger.info(f"Новый документ сохранен: {document['url']}")
                return True
                
        except Exception as e:
            self.logger.error(f"Ошибка сохранения документа: {e}")
            return False
    
    def run(self):
        """Основной цикл работы краулера"""
        self.logger.info("Запуск поискового робота...")
        
        
        last_url = self.load_checkpoint()
        
        
        all_urls = []
        
        for source_name, source_config in self.config['sources'].items():
            if source_config.get('enabled', True):
                self.logger.info(f"Обработка источника: {source_name}")
                
                urls = self.get_urls_from_source(source_name, source_config)
                max_docs = self.config['logic'].get('max_documents_per_source', 100)
                
                
                urls = urls[:max_docs]
                
                
                for url in urls:
                    all_urls.append({
                        'url': url,
                        'source_name': source_config['source_name'],
                        'respect_robots_txt': source_config.get(
                            'respect_robots_txt',
                            self.config['robots_txt'].get('respect_robots_txt', True)
                        )
                    })
        
        self.logger.info(f"Всего URL для обкачки: {len(all_urls)}")
        
        
        start_index = 0
        if last_url:
            for i, url_info in enumerate(all_urls):
                if url_info['url'] == last_url:
                    start_index = i + 1
                    self.logger.info(f"Продолжаем с позиции {start_index}")
                    break
        
        
        for i in range(start_index, len(all_urls)):
            if not self.running:
                self.logger.info("Остановка краулера по запросу")
                break
            
            url_info = all_urls[i]
            url = url_info['url']
            source_name = url_info['source_name']
            respect_robots = url_info.get('respect_robots_txt', True)
            
            
            existing = self.collection.find_one({'url': url})
            if existing and not self.should_recrawl(existing):
                self.stats['skipped_documents'] += 1
                self.logger.debug(f"Пропускаем (слишком рано для переобкачки): {url}")
                continue
            
            
            document = self.crawl_url(url, source_name, respect_robots)
            
            if document:
                
                success = self.process_document(document)
                
                if success:
                    self.stats['total_crawled'] += 1
                    
                    
                    self.save_checkpoint(url)
                    
                    
                    if self.stats['total_crawled'] % 10 == 0:
                        self.print_statistics()
                
            
            delay = self.config['logic'].get('delay_between_requests', 2.0)
            time.sleep(delay)
        
        
        self.print_statistics()
        self.save_final_statistics()
        
        self.logger.info("Работа поискового робота завершена")
    
    def print_statistics(self):
        """Вывод текущей статистики"""
        elapsed = time.time() - self.stats['start_time']
        
        stats_str = f"""
        === СТАТИСТИКА КРАУЛЕРА ===
        Всего обработано: {self.stats['total_crawled']}
        Новых документов: {self.stats['new_documents']}
        Обновленных документов: {self.stats['updated_documents']}
        Пропущенных документов: {self.stats['skipped_documents']}
        Ошибок обкачки: {self.stats['failed_documents']}
        Прошедшее время: {elapsed:.1f} секунд
        Скорость: {self.stats['total_crawled']/max(elapsed, 1):.2f} документов/секунду
        ============================
        """
        
        self.logger.info(stats_str)
    
    def save_final_statistics(self):
        """Сохранение финальной статистики"""
        try:
            stats_collection = self.db['crawler_statistics']
            
            stats_data = {
                'timestamp': time.time(),
                'stats': self.stats,
                'config': {
                    'sources': list(self.config['sources'].keys()),
                    'delay': self.config['logic'].get('delay_between_requests')
                }
            }
            
            stats_collection.insert_one(stats_data)
            
            self.logger.info("Финальная статистика сохранена")
            
        except Exception as e:
            self.logger.error(f"Ошибка сохранения статистики: {e}")
    
    def stop(self):
        """Остановка краулера"""
        self.running = False
        self.logger.info("Получен сигнал остановки")


def signal_handler(signum, frame):
    """Обработчик сигналов для graceful shutdown"""
    print("\nПолучен сигнал остановки...")
    if 'crawler' in globals():
        crawler.stop()


if __name__ == "__main__":
    import signal
    
    
    if len(sys.argv) != 2:
        print("Использование: python robot.py <путь_к_config.yaml>")
        print("Пример: python robot.py config.yaml")
        sys.exit(1)
    
    config_path = sys.argv[1]
    
    
    crawler = MusicCrawler(config_path)
    
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        crawler.run()
    except KeyboardInterrupt:
        print("\nКраулер остановлен пользователем")
        crawler.stop()
    except Exception as e:
        print(f"Критическая ошибка: {e}")
        import traceback
        traceback.print_exc()